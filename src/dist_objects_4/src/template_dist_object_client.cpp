//  Copyright (c) 2019 Weile Wei
//  Copyright (c) Maxwell Resser
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

#include "template_dist_object.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

REGISTER_PARTITION(int);
using myVectorInt = std::vector<int>;
REGISTER_PARTITION(myVectorInt);

void run_dist_object_vector() {
	// define vector based on the locality that it is running
	int here_ = static_cast<int>(hpx::get_locality_id());
	int len = 10;
	std::vector<int> a(len, here_);
	std::vector<int> b(len, here_);
	std::vector<int> c(len, 0);

	// construct int type dist_objects to be used later
	dist_object::dist_object<int> A(hpx::find_here(), a);
	dist_object::dist_object<int> B(hpx::find_here(), b);
	dist_object::dist_object<int> C(hpx::find_here(), c);

	// perform element-wise addition between dist_objects
	for (int i = 0; i < len; i++) {
		(*C)[i] = (*A)[i] + (*B)[i];
	}

	for (int i = 0; i < len; i++) {
		c[i] = a[i] + b[i];
	}

	assert((*C) == c);
}

void run_dist_object_matrix() {
	int val = 42 + static_cast<int>(hpx::get_locality_id());
	int rows = 5, cols = 5;

	std::vector<std::vector<int>> m1(rows, std::vector<int>(cols, val));
	std::vector<std::vector<int>> m2(rows, std::vector<int>(cols, val));
	std::vector<std::vector<int>> m3(rows, std::vector<int>(cols, 0));

	dist_object::dist_object<std::vector<int>> M1(hpx::find_here(), m1);
	dist_object::dist_object<std::vector<int>> M2(hpx::find_here(), m2);
	dist_object::dist_object<std::vector<int>> M3(hpx::find_here(), m3);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			(*M3)[i][j] = (*M1)[i][j] + (*M2)[i][j];
		}
	}

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			m3[i][j] = m1[i][j] + m2[i][j];
		}
	}
	assert((*M3) == m3);
}

int hpx_main() {
	run_dist_object_vector();
	run_dist_object_matrix();
	return hpx::finalize();
}

int main(int argc, char *argv[]) { return hpx::init(argc, argv); }
