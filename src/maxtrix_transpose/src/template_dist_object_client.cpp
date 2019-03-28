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

void transpose(hpx::future<std::vector<std::vector<double> > > Af, std::uint64_t M1_offset,
	dist_object::dist_object<std::vector<double>>& Bf, std::uint64_t M2_offset, std::uint64_t block_size,
	std::uint64_t block_order, std::uint64_t tile_size);

void transpose_local(dist_object::dist_object<std::vector<double>>& Af, std::uint64_t M1_offset,
	dist_object::dist_object<std::vector<double>>& Bf, std::uint64_t M2_offset, std::uint64_t block_size,
	std::uint64_t block_order, std::uint64_t tile_size);

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
	std::uint64_t order = 2;
	std::uint64_t col_block_size = 2;
	std::uint64_t num_blocks = 2;
	std::uint64_t block_order = 1;
	std::uint64_t tile_size = block_order;
	std::uint64_t num_local_blocks = 1;
	std::uint64_t blocks_start = id * num_local_blocks;
	std::uint64_t blocks_end = (id + 1) * num_local_blocks;

	std::vector<double> m1{ 0.0, 1.0 };
	std::vector<double> m2{ -1.0, -1.0 };
	dist_object::dist_object<double> M1("m1", m1);
	dist_object::dist_object<double> M2("m2", m2);

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
				if (num_blocks == hpx::get_locality_id()) {
					phase_futures.push_back(
						hpx::dataflow(
							&transpose
							, M1
							, M1_offet
							, M2
							, M2_offset
							, block_size
						)
					);
				}
				// fetch remote data and transpose matrix locally
				else {

				}

				phase_futures.push_back(
					hpx::dataflow(
						&transpose
						, A[from_block].get_sub_block(A_offset, block_size)
						, B[b].get_sub_block(B_offset, block_size)
						, block_order
						, tile_size
					)
				);
			}

			block_futures[b - blocks_start] =
				hpx::when_all(phase_futures);
		}
	);

	hpx::wait_all(block_futures);

	//std::vector<int> range{ 0}; // auto range = boost::irange(blocks_start, blocks_end);
	//std::vector<hpx::future<void> > block_futures;
	//block_futures.resize(1); // num_local_blocks
	//for_each(par, std::begin(range), std::end(range),
	//	[&](std::uint64_t b) 
	//{
	//	std::vector<hpx::future<void> > phase_futures;
	//	phase_futures.reserve(num_blocks); //num_blocks
	//	std::vector<int> phase_range{ 0 }; // auto phase_range = boost::irange(static_cast<std::uint64_t>(0), num_blocks);
	//	for (std::uint64_t phase : phase_range) {
	//		const std::uint64_t block_size = block_order * block_order;
	//		const std::uint64_t from_block = phase;
	//		const std::uint64_t from_phase = b;
	//		const std::uint64_t M1_offset = from_phase * block_size;
	//		const std::uint64_t M2_offset = phase * block_size;

	//		if (from_block == hpx::get_locality_id())
	//		{
	//			phase_futures.push_back
	//			(
	//				hpx::dataflow(
	//					&transpose_local,
	//					M1,
	//					M1_offset,
	//					M2,
	//					M2_offset,
	//					block_size,
	//					block_order,
	//					tile_size
	//			));
	//		}
	//		else 
	//		{
	//			phase_futures.push_back
	//			(
	//				hpx::dataflow(
	//					&transpose,
	//					M1.fetch(from_block),
	//					M1_offset,
	//					M2,
	//					M2_offset,
	//					block_size,
	//					block_order,
	//					tile_size
	//				));
	//		}
	//		block_futures[b - 0] =
	//			hpx::when_all(phase_futures);
	//	}
	//});
	//hpx::wait_all(block_futures);
	//if (hpx::get_locality_id() == 0) {
	//	transpose_local(M1, 0, M2, 0, 1, 1, 1);
	//	//(*M2)[0][0] = (*M1)[0][0];
	//	(*M2)[0][1] = (M1.fetch(1)).get()[0][0];
	//}
	//else {
	//	(*M2)[0][0] = (M1.fetch(0)).get()[0][1];
	//	(*M2)[0][1] = (*M1)[0][1];
	//}

	//std::vector<int> locality_range;
	//locality_range.reserve(num_localities);
	//for (int i = 0; i < num_localities; i++) {
	//	locality_range.push_back(i);
	//}

	//for (std::uint64_t locality : locality_range) {

	//}



	//std::uint64_t num_local_blocks = num_blocks;
	//std::uint64_t tile_size = order;

	//std::uint64_t num_blocks = num_localities * num_local_blocks;

	//std::uint64_t block_order = order / num_blocks;
	//std::uint64_t col_block_size = order * block_order;

	//std::uint64_t id = hpx::get_locality_id();

	//std::vector<std::vector<double>> m1(order, std::vector<double>(col_block_size));
	//std::vector<std::vector<double>> m2(order, std::vector<double>(col_block_size));

	//// Fill the original matrix, set transpose to known garbage value.

	//std::vector<int> range{0, 1};
	//assert(range.size() == num_blocks);

	//for (std::uint64_t b = 0; b < num_blocks; b++) {
	//	for (std::uint64_t i = 0; i < order; ++i)
	//	{
	//		for (std::uint64_t j = 0; j < block_order; ++j)
	//		{
	//			double col_val = COL_SHIFT * (b*block_order + j);

	//			m1[b][i * block_order + j] = col_val + ROW_SHIFT * i;
	//			m2[b][i * block_order + j] = -1.0;
	//			std::cout << m1[b][i * block_order + j] << " ";
	//		}
	//	}
	//	std::cout << std::endl;
	//}
	//for (int i = 0; i < m1.size(); i++) {
	//	for (int j = 0; j < m1[0].size(); j++) {
	//		std::cout << m1[i][j] << " ";
	//	}
	//	std::cout << std::endl;
	//}

	//dist_object::dist_object<std::vector<double>> M1("m1", m1);
	//dist_object::dist_object<std::vector<double>> M2("m2", m2);

	//std::vector<hpx::future<void> > block_futures;
	//block_futures.resize(2);
	//
	//using hpx::parallel::for_each;
	//using hpx::parallel::execution::par;

	//hpx::parallel::for_each(par, std::begin(range), std::end(range),
	//	[&](std::uint64_t b)
	//{
	//	std::vector<hpx::future<void> > phase_futures;
	//	phase_futures.reserve(num_blocks);

	//	std::vector<int> phase_range{ 0, 1 };

	//	for (std::uint64_t phase : phase_range)
	//	{
	//		const std::uint64_t block_size = block_order * block_order;
	//		const std::uint64_t from_block = phase;
	//		const std::uint64_t from_phase = b;
	//		const std::uint64_t A_offset = from_phase * block_size;
	//		const std::uint64_t B_offset = phase * block_size;

	//		phase_futures.push_back(
	//			hpx::dataflow(
	//				&transpose,
	//				M1.fetch(0), from_block, M2, from_phase, block_size, block_order, tile_size
	//			));
	//	}

	//	block_futures[b - 0] =
	//		hpx::when_all(phase_futures);
	//});

	//hpx::wait_all(block_futures);


	//const std::uint64_t block_size = block_order * block_order;
	//std::vector<hpx::future<void> > phase_futures;
	//phase_futures.reserve(num_blocks);
	//phase_futures.push_back(
	//	hpx::dataflow(
	//		&transpose,
	//		M1.fetch(0), 0, M2, 0, block_size, block_order, tile_size
	//));
	//hpx::when_all(phase_futures);

	//phase_futures.push_back(
	//	hpx::dataflow(
	//		&transpose,
	//		M1.fetch(0), 1, M2, 0, block_size, block_order, tile_size
	//	));
	//hpx::when_all(phase_futures);

	for (int i = 0; i < (*M2).size(); i++) {
		for (int j = 0; j < (*M2)[0].size(); j++) {
			hpx::cout << (*M2)[i][j] << " ";
		}
		hpx::cout << std::endl;
	}

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
	//				, M1_offset
	//				, M2.fetch(b)
	//				, M2_offset
	//				, block_size
	//				, block_order
	//				, tile_size
	//			)
	//		);
	//	}
	//	block_futures[b - 0] =
	//		hpx::when_all(phase_futures);
	//}
}

void transpose(hpx::future<std::vector<std::vector<double> > > Af, std::uint64_t M1_offset, 
	dist_object::dist_object<std::vector<double>>& Bf, std::uint64_t M2_offset, std::uint64_t block_size,
	std::uint64_t block_order, std::uint64_t tile_size)
{
	const std::vector<std::vector<double>> aa(Af.get());

	const std::vector<double> A = aa[M1_offset];

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
						(*Bf)[M2_offset][it + block_order * jt] = A[jt + block_order * it];
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
				(*Bf)[M2_offset][i + block_order * j] = A[j + block_order * i];
			}
		}
	}
}

void transpose_local(dist_object::dist_object<std::vector<double>>& Af, std::uint64_t M1_offset,
	dist_object::dist_object<std::vector<double>>& Bf, std::uint64_t M2_offset, std::uint64_t block_size,
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
						(*Bf)[M2_offset][it + block_order * jt] = (*Af)[M1_offset][jt + block_order * it];
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
				(*Bf)[M2_offset][i + block_order * j] = (*Af)[M1_offset][j + block_order * i];
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
