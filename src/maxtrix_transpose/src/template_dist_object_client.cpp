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

#include "template_dist_object.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

//void transpose(hpx::future<std::vector<std::vector<double> > > A, hpx::future<std::vector<std::vector<double> > > B,
//	std::uint64_t block_order, std::uint64_t tile_size);

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

	std::uint64_t num_localities = hpx::get_num_localities().get();
	std::uint64_t order = 2;
	std::uint64_t iterations = 1;
	std::uint64_t num_local_blocks = 2;
	std::uint64_t tile_size = order;

	std::uint64_t num_blocks = num_localities * num_local_blocks;

	std::uint64_t block_order = order / num_blocks;
	std::uint64_t col_block_size = order * block_order;

	std::uint64_t id = hpx::get_locality_id();

	std::vector<std::vector<double>> m1(order, std::vector<double>(col_block_size));
	std::vector<std::vector<double>> m2(order, std::vector<double>(col_block_size));

	// Fill the original matrix, set transpose to known garbage value.

	std::vector<int> range{0, 1};
	assert(range.size() == num_blocks);

	for (std::uint64_t b = 0; b < num_blocks; b++) {
		for (std::uint64_t i = 0; i < order; ++i)
		{
			for (std::uint64_t j = 0; j < block_order; ++j)
			{
				double col_val = COL_SHIFT * (b*block_order + j);

				m1[b][i * block_order + j] = col_val + ROW_SHIFT * i;
				m2[b][i * block_order + j] = -1.0;
				std::cout << m1[b][i * block_order + j] << " ";
			}
		}
		std::cout << std::endl;
	}
	for (int i = 0; i < m1.size(); i++) {
		for (int j = 0; j < m1[0].size(); j++) {
			std::cout << m1[i][j] << " ";
		}
		std::cout << std::endl;
	}

	//std::vector<std::vector<double>> m3(2, std::vector<double>(2, 2.0));
	//dist_object::dist_object<std::vector<double>> M3("m3", m3);
	dist_object::dist_object<std::vector<double>> M1("m1", m1);
	dist_object::dist_object<std::vector<double>> M2("m2", m2);

	//std::vector<hpx::future<void> > block_futures;
	//block_futures.resize(num_local_blocks);


	//for (std::uint64_t b = 0; b < num_blocks; b++) {
	//	std::vector<hpx::future<void> > phase_futures;
	//	phase_futures.reserve(num_blocks);

	//	for (std::uint64_t phase = 0; phase < num_blocks; phase++) {
	//		const std::uint64_t block_size = block_order * block_order;
	//		const std::uint64_t from_block = phase;
	//		const std::uint64_t from_phase = b;
	//		const std::uint64_t M1_offset = from_phase * block_size;
	//		const std::uint64_t M2_offset = phase * block_size;

	//		phase_futures.push_back(
	//			hpx::dataflow(
	//				&transpose
	//				, M1.fetch(from_block)
	//				, M2.fetch(b)
	//				, block_order
	//				, tile_size
	//			)
	//		);
	//	}
	//	block_futures[b - 0] =
	//		hpx::when_all(phase_futures);
	//}
}

//void transpose(hpx::future<std::vector<std::vector<double> > > Af, hpx::future<std::vector<std::vector<double> > > Bf,
//	std::uint64_t block_order, std::uint64_t tile_size)
//{
//	const std::vector<double> A(Af.get());
//	std::vector<double> B(Bf.get());
//
//	if (tile_size < block_order)
//	{
//		for (std::uint64_t i = 0; i < block_order; i += tile_size)
//		{
//			for (std::uint64_t j = 0; j < block_order; j += tile_size)
//			{
//				std::uint64_t max_i = (std::min)(block_order, i + tile_size);
//				std::uint64_t max_j = (std::min)(block_order, j + tile_size);
//
//				for (std::uint64_t it = i; it != max_i; ++it)
//				{
//					for (std::uint64_t jt = j; jt != max_j; ++jt)
//					{
//						B[it + block_order * jt] = A[jt + block_order * it];
//					}
//				}
//			}
//		}
//	}
//	else
//	{
//		for (std::uint64_t i = 0; i != block_order; ++i)
//		{
//			for (std::uint64_t j = 0; j != block_order; ++j)
//			{
//				B[i + block_order * j] = A[j + block_order * i];
//			}
//		}
//	}
//}

int hpx_main() {
	run_matrix_transposition();
	std::cout << "Hello world from locality " << hpx::get_locality_id() << std::endl;
	return hpx::finalize();
}

int main(int argc, char *argv[]) { return hpx::init(argc, argv); }
