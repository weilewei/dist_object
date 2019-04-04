//  Copyright (c) 2019 Weile Wei
//  Copyright (c) 2019 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////
/// This is a dist_object example using HPX component.
///
/// A distributed object is a single logical object partitioned across 
/// a set of localities. (A locality is a single node in a cluster or a 
/// NUMA domian in a SMP machine.) Each locality constructs an instance of
/// dist_object<T>, where a value of type T represents the value of this
/// this locality's instance value. Once dist_object<T> is conctructed, it
/// has a universal name which can be used on any locality in the given
/// localities to locate the resident instance.

#include <hpx/hpx_init.hpp>
#include <hpx/include/iostreams.hpp>
#include <hpx/lcos/barrier.hpp>
#include <hpx/lcos/dataflow.hpp>
#include <hpx/lcos/when_all.hpp>
#include <hpx/parallel/algorithms/for_each.hpp>

#include "template_dist_object.hpp"

#include <boost/range/irange.hpp>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

typedef double* sub_block;

void transpose(hpx::future<std::vector<double>> M1, std::uint64_t M1_offset,
	dist_object::dist_object<double> &M2, std::uint64_t M2_offset,
	std::uint64_t block_size, std::uint64_t block_order,
	std::uint64_t tile_size);

void transpose_local(dist_object::dist_object<double> &M1,
	std::uint64_t M1_offset,
	dist_object::dist_object<double> &M2,
	std::uint64_t M2_offset, std::uint64_t block_size,
	std::uint64_t block_order, std::uint64_t tile_size);

#define COL_SHIFT 1000.00           // Constant to shift column index
#define ROW_SHIFT 0.01             // Constant to shift row index

REGISTER_PARTITION(int);
REGISTER_PARTITION(double);
using myVectorInt = std::vector<int>;
REGISTER_PARTITION(myVectorInt);
using myVectorDouble = std::vector<double>;
REGISTER_PARTITION(myVectorDouble);

void run_matrix_transposition(boost::program_options::variables_map& vm) {
	hpx::id_type here = hpx::find_here();
	bool root = here == hpx::find_root_locality();

	std::uint64_t num_localities = hpx::get_num_localities().get();

	std::uint64_t order = vm["matrix_size"].as<std::uint64_t>();
	std::uint64_t iterations = vm["iterations"].as<std::uint64_t>();
	std::uint64_t num_local_blocks = vm["num_blocks"].as<std::uint64_t>();
	std::uint64_t tile_size = order;

	if (vm.count("tile_size"))
		tile_size = vm["tile_size"].as<std::uint64_t>();

	//verbose = vm.count("verbose") ? true : false;

	std::uint64_t bytes =
		static_cast<std::uint64_t>(2.0 * sizeof(double) * order * order);

	std::uint64_t num_blocks = num_localities * num_local_blocks;

	std::uint64_t block_order = order / num_blocks;
	std::uint64_t col_block_size = order * block_order;

	std::uint64_t id = hpx::get_locality_id();

	std::vector<dist_object::dist_object<double>> A(num_blocks);
	std::vector<dist_object::dist_object<double>> B(num_blocks);

	std::uint64_t blocks_start = id * num_local_blocks;
	std::uint64_t blocks_end = (id + 1) * num_local_blocks;

	// First allocate and create our local blocks
	for (std::uint64_t b = 0; b != num_local_blocks; ++b)
	{
		std::uint64_t block_idx = b + blocks_start;
		A[block_idx] = dist_object::dist_object<double>("A", std::vector<double>(col_block_size));
		B[block_idx] = dist_object::dist_object<double>("B", std::vector<double>(col_block_size));
	}

	using hpx::parallel::for_each;
	using hpx::parallel::execution::par;
	using hpx::parallel::execution::seq;

	// Fill the original matrix, set transpose to known garbage value.
	auto range = boost::irange(blocks_start, blocks_end);
	hpx::parallel::for_each(par, std::begin(range), std::end(range),
		[&](std::uint64_t b)
	{
		for (std::uint64_t i = 0; i != order; ++i)
		{
			for (std::uint64_t j = 0; j != block_order; ++j)
			{
				double col_val = COL_SHIFT * (b*block_order + j);
				(*A[b])[i * block_order + j] = col_val + ROW_SHIFT * i;
				(*B[b])[i * block_order + j] = -1.0;
			}
		}
	}
	);

	// wait all matrix to be initialized
	hpx::lcos::barrier b("wait_for_init", hpx::find_all_localities().size(), hpx::get_locality_id());
	b.wait();

	if (root)
	{
		hpx::cout
			<< "Serial Matrix transpose: B = A^T\n"
			<< "Matrix order          = " << order << "\n"
			<< "Matrix local columns  = " << block_order << "\n"
			<< "Number of blocks      = " << num_blocks << "\n"
			<< "Number of localities  = " << num_localities << "\n";
		if (tile_size < order)
			hpx::cout << "Tile size             = " << tile_size << "\n";
		else
			hpx::cout << "Untiled\n";
		hpx::cout
			<< "Number of iterations  = " << iterations << "\n";
	}

	double errsq = 0.0;
	double avgtime = 0.0;
	double maxtime = 0.0;
	double mintime = 366.0 * 24.0*3600.0; // set the minimum time to a large value;
										  // one leap year should be enough

	// starts matrix transposition
	std::vector<hpx::future<void> > block_futures;
	block_futures.resize(num_local_blocks);
	hpx::util::high_resolution_timer t;
	for_each(par, std::begin(range), std::end(range),
		[&](std::uint64_t b)
	{
		std::vector<hpx::future<void> > phase_futures;
		phase_futures.reserve(num_blocks);

		auto phase_range = boost::irange(
			static_cast<std::uint64_t>(0), num_blocks);
		for (std::uint64_t phase : phase_range)
		{
			const std::uint64_t block_size = block_order * block_order;
			const std::uint64_t from_block = phase;
			const std::uint64_t from_phase = b;
			const std::uint64_t A_offset = from_phase * block_size;
			const std::uint64_t B_offset = phase * block_size;
			const std::uint64_t from_locality = from_block % num_localities;
			// local matrix transpose
			// TODO: only fetch one time, but, what if it is able to fetch one time, the change
			// from origin will not be updated next time.
			//hpx::cout << " block_size: " << block_size
			//	<< " from_block: " << from_block
			//	<< " from_phase: " << from_phase
			//	<< " A_offset: " << A_offset
			//	<< " B_offset: " << B_offset
			//	<< "\n";
			if (blocks_start <= phase && phase < blocks_end) {
				phase_futures.push_back(
				hpx::async(&transpose_local
					, A[from_block]
					, A_offset
					, B[b]
					, B_offset
					, block_size
					, block_order
					, tile_size
					)
				);
			}
			// fetch remote data and transpose matrix locally
			else {
				phase_futures.push_back(
					hpx::dataflow(
						&transpose
						, A[b].fetch(from_locality)
						, A_offset
						, B[b]
						, B_offset
						, block_size
						, block_order
						, tile_size
					)
				);
			}
		}

		block_futures[b - blocks_start] =
			hpx::when_all(phase_futures);
	}
	);

	hpx::wait_all(block_futures);

	double epsilon = 1.e-8;
	if (root)
	{
		if (errsq < epsilon)
		{
			hpx::cout << "Solution validates\n";
			avgtime = avgtime / static_cast<double>(
				(std::max)(iterations - 1, static_cast<std::uint64_t>(1)));
			hpx::cout
				<< "Rate (MB/s): " << 1.e-6 * bytes / mintime << ", "
				<< "Avg time (s): " << avgtime << ", "
				<< "Min time (s): " << mintime << ", "
				<< "Max time (s): " << maxtime << "\n";

				hpx::cout << "Squared errors: " << errsq << "\n";
		}
		else
		{
			hpx::cout
				<< "ERROR: Aggregate squared error " << errsq
				<< " exceeds threshold " << epsilon << "\n";
			hpx::terminate();
		}
	}
}

void transpose(hpx::future<std::vector<double> > M1f, std::uint64_t M1_offset,
	dist_object::dist_object<double>& M2, std::uint64_t M2_offset, std::uint64_t block_size,
	std::uint64_t block_order, std::uint64_t tile_size)
{
	std::vector<double> AA = M1f.get();
	const sub_block A(&(AA[M1_offset]));
	sub_block B(&((*M2)[M2_offset]));
	if (tile_size < block_order)
	{
		for (std::uint64_t i = 0; i < block_order; i += tile_size)
		{
			for (std::uint64_t j = 0; j < block_order; j += tile_size)
			{
				std::uint64_t max_i = (std::min)(block_order, i + tile_size);
				std::uint64_t max_j = (std::min)(block_order, j + tile_size);

				for (std::uint64_t it = i; it != max_i; ++it)
				{
					for (std::uint64_t jt = j; jt != max_j; ++jt)
					{
						B[it + block_order * jt] = A[jt + block_order * it];
					}
				}
			}
		}
	}
	else
	{
		for (std::uint64_t i = 0; i != block_order; ++i)
		{
			for (std::uint64_t j = 0; j != block_order; ++j)
			{
				B[i + block_order * j] = A[j + block_order * i];
			}
		}
	}
}

void transpose_local(dist_object::dist_object<double>& M1, std::uint64_t M1_offset,
	dist_object::dist_object<double>& M2, std::uint64_t M2_offset,
	std::uint64_t block_size, std::uint64_t block_order, std::uint64_t tile_size)
{
	const sub_block A(&((*M1)[M1_offset]));
	sub_block B(&((*M2)[M2_offset]));
	if (tile_size < block_order)
	{
		for (std::uint64_t i = 0; i < block_order; i += tile_size)
		{
			for (std::uint64_t j = 0; j < block_order; j += tile_size)
			{
				std::uint64_t max_i = (std::min)(block_order, i + tile_size);
				std::uint64_t max_j = (std::min)(block_order, j + tile_size);

				for (std::uint64_t it = i; it != max_i; ++it)
				{
					for (std::uint64_t jt = j; jt != max_j; ++jt)
					{
						B[it + block_order * jt] = A[jt + block_order * it];
					}
				}
			}
		}
	}
	else
	{
		for (std::uint64_t i = 0; i != block_order; ++i)
		{
			for (std::uint64_t j = 0; j != block_order; ++j)
			{
				B[i + block_order * j] = A[j + block_order * i];
			}
		}
	}
}

int hpx_main(boost::program_options::variables_map& vm) {
	run_matrix_transposition(vm);
	return hpx::finalize();
}

int main(int argc, char *argv[])
{
	using namespace boost::program_options;

	options_description desc_commandline;
	desc_commandline.add_options()
		("matrix_size", value<std::uint64_t>()->default_value(4),
		"Matrix Size")
		("iterations", value<std::uint64_t>()->default_value(1),
		"# iterations")
		("tile_size", value<std::uint64_t>(),
		"Number of tiles to divide the individual matrix blocks for improved "
		"cache and TLB performance")
		("num_blocks", value<std::uint64_t>()->default_value(2),
		"Number of blocks to divide the individual matrix blocks for "
		"improved cache and TLB performance")
		("verbose", "Verbose output")
		;

	// Initialize and run HPX, this example requires to run hpx_main on all
	// localities
	std::vector<std::string> const cfg = {
		"hpx.run_hpx_main!=1"
	};
	return hpx::init(desc_commandline, argc, argv, cfg);
}
