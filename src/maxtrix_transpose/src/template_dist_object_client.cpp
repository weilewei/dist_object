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

#include <hpx/lcos/dataflow.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/iostreams.hpp>
#include <hpx/lcos/barrier.hpp>
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

void transpose(hpx::future<std::vector<double> > Af, std::uint64_t M1_offset,
	dist_object::dist_object<double>& Bf, std::uint64_t M2_offset, std::uint64_t block_size,
	std::uint64_t block_order, std::uint64_t tile_size);

void transpose_local(dist_object::dist_object<double>& M1, std::uint64_t M1_offset, dist_object::dist_object<double>& M2, std::uint64_t M2_offset, std::uint64_t block_size, std::uint64_t block_order, std::uint64_t tile_size);

#define COL_SHIFT 1000.00           // Constant to shift column index
#define ROW_SHIFT 0.01             // Constant to shift row index

REGISTER_PARTITION(int);
REGISTER_PARTITION(double);
using myVectorInt = std::vector<int>;
REGISTER_PARTITION(myVectorInt);
using myVectorDouble = std::vector<double>;
REGISTER_PARTITION(myVectorDouble);

void run_matrix_transposition() {
	hpx::id_type here = hpx::find_here();
	bool root = here == hpx::find_root_locality();
	std::uint64_t id = hpx::get_locality_id();
	std::uint64_t num_localities = hpx::get_num_localities().get();
	std::uint64_t order = 4;
	std::uint64_t num_local_blocks = 2;

	std::uint64_t num_blocks = num_localities * num_local_blocks; // 4
	std::uint64_t block_order = order / num_blocks; // 1
	std::uint64_t col_block_size = order * block_order; // 4
	std::uint64_t tile_size = order;

	std::uint64_t blocks_start = id * num_local_blocks;
	std::uint64_t blocks_end = (id + 1) * num_local_blocks;

	dist_object::dist_object<double> M1("m1", std::vector<double>(8));
	dist_object::dist_object<double> M2("m2", std::vector<double>(8));

	using hpx::parallel::for_each;
	using hpx::parallel::execution::par;
	using hpx::parallel::execution::seq;
	auto range = boost::irange(blocks_start, blocks_end);
	for_each(seq, std::begin(range), std::end(range),
		[&](std::uint64_t b)
	{
		for (std::uint64_t i = 0; i != order; ++i)
		{
			for (std::uint64_t j = 0; j != block_order; ++j)
			{
				double col_val = COL_SHIFT * (b*block_order + j);
				(*M1)[(b % 2) * order + i * block_order + j] = col_val + ROW_SHIFT * i;
				(*M2)[(b % 2) * order + i * block_order + j] = -1.0;
			}
		}
	}
	);

	//for (size_t i = 0; i < (*M1).size(); i++)
	//{
	//	hpx::cout << std::to_string((*M1)[i]) << " ";
	//}
	//hpx::cout << "\n";

	std::vector<hpx::future<void> > block_futures;
	block_futures.resize(num_local_blocks);
	for_each(seq, std::begin(range), std::end(range),
		[&](std::uint64_t b)
	{
		std::vector<hpx::future<void> > phase_futures;
		phase_futures.reserve(num_blocks);

		auto phase_range = boost::irange(
			static_cast<std::uint64_t>(0), num_blocks);
		for (std::uint64_t phase : phase_range)
		{
			const std::uint64_t block_size = block_order * block_order; // 1
			const std::uint64_t from_locality = phase / num_local_blocks;
			//const std::uint64_t from_block = phase; // 0 -4
			//const std::uint64_t from_phase = b; // 0 - 2
			const std::uint64_t M1_offset = b + (phase % 2) * 4;
			const std::uint64_t M2_offset = (b % 2) * 4 + phase;
			hpx::cout << std::to_string(M1_offset) << " " << std::to_string(M2_offset) << "\n";
			// local matrix transpose
			if (blocks_start <= phase && phase < blocks_end) {
				phase_futures.push_back(
					hpx::dataflow(
						&transpose_local
						, M1
						, M1_offset
						, M2
						, M2_offset
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
						, M1.fetch(from_locality)
						, M1_offset
						, M2
						, M2_offset
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

	for (size_t i = 0; i < (*M2).size(); i++)
	{
		hpx::cout << std::to_string((*M2)[i]) << " ";
	}
	hpx::cout << "\n";

}

void transpose(hpx::future<std::vector<double> > Af, std::uint64_t M1_offset,
	dist_object::dist_object<double>& Bf, std::uint64_t M2_offset, std::uint64_t block_size,
	std::uint64_t block_order, std::uint64_t tile_size)
{
	std::vector<double> AA = Af.get();
	const sub_block A(&(AA[M1_offset]));
	sub_block B(&((*Bf)[M2_offset]));

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

void transpose_local(dist_object::dist_object<double>& M1, std::uint64_t M1_offset, dist_object::dist_object<double>& M2, std::uint64_t M2_offset, std::uint64_t block_size,
	std::uint64_t block_order, std::uint64_t tile_size)
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



int hpx_main() {
	run_matrix_transposition();
	hpx::cout << "Hello world from locality " << hpx::get_locality_id() << hpx::endl;
	return hpx::finalize();
}

int main(int argc, char *argv[]) { return hpx::init(argc, argv); }
