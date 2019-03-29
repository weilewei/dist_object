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

void transpose_local(sub_block A, sub_block B, std::uint64_t block_size, std::uint64_t block_order, std::uint64_t tile_size);

#define COL_SHIFT 1000.00           // Constant to shift column index
#define ROW_SHIFT 0.001             // Constant to shift row index

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

	
	std::uint64_t num_blocks = num_localities * num_local_blocks;
	std::uint64_t block_order = order / num_blocks;
	std::uint64_t col_block_size = order * block_order;
	std::uint64_t tile_size = order;
	
	std::uint64_t blocks_start = id * num_local_blocks;
	std::uint64_t blocks_end = (id + 1) * num_local_blocks;

	std::vector<double> m1{ 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 };
	std::vector<double> m2{ -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0};
	dist_object::dist_object<double> M1("m1", m1);
	dist_object::dist_object<double> M2("m2", m2);

	//for (int i = 0; i < (*M2).size(); i++) {
	//	hpx::cout << (*M2)[i] << " ";
	//}
	hpx::cout << "\n";
	
	using hpx::parallel::for_each;
	using hpx::parallel::execution::par;

	auto range = boost::irange(blocks_start, blocks_end);
	std::vector<hpx::future<void> > block_futures;
	block_futures.resize(num_local_blocks);
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
				const std::uint64_t M1_offset = from_phase * block_size;
				const std::uint64_t M2_offset = phase * block_size;

				// local matrix transpose
				if (blocks_start <= phase && phase<blocks_end) {
					phase_futures.push_back(
						hpx::dataflow(
							&transpose_local
							, &((*M1)[M1_offset])
							, &((*M2)[M2_offset])
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
							, M1.fetch(from_block/ num_local_blocks)
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

	for (int i = 0; i < (*M2).size(); i++) {
			hpx::cout << (*M2)[i] << " ";
	}
	hpx::cout << "\n";
}

void transpose(hpx::future<std::vector<double> > Af, std::uint64_t M1_offset, 
	dist_object::dist_object<double>& Bf, std::uint64_t M2_offset, std::uint64_t block_size,
	std::uint64_t block_order, std::uint64_t tile_size)
{
	std::vector<double> AA = Af.get();
	const sub_block A(&(AA[M1_offset]));
	const sub_block B(&((*Bf)[M2_offset]));

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

void transpose_local(sub_block A, sub_block B, std::uint64_t block_size,
	std::uint64_t block_order, std::uint64_t tile_size)
{
	
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
