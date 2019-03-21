//  Copyright (c) 2019 Weile Wei
//  Copyright (c) Maxwell Resser
//  Copyright (c) 2019 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_init.hpp>
#include <hpx/include/iostreams.hpp>

#include "template_dist_object.hpp"

#include <iostream>
#include <string>
#include <vector>

REGISTER_PARTITION(int);
using myVectorInt = std::vector<int>;

void run_dist_object_vector() {
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

	// define vector based on the locality that it is running
	int here_ = static_cast<int>(hpx::get_locality_id());
	std::vector<int> a(10, here_);
	std::vector<int> b(10, here_);
	std::vector<int> c(10);

	// construct int type dist_objects to be used later
	dist_object::dist_object<int> A(hpx::find_here(), a);
	dist_object::dist_object<int> B(hpx::find_here(), b);

	// perform element-wise addition between dist_objects
	for (int i = 0; i < (*A).size(); i++) {
		c[i] = (*A)[i] + (*B)[i];
	}

	// construct the result
	dist_object::dist_object<int> C(hpx::find_here(), c);

	// print result
	C.print();
	return;
}

int hpx_main() {
	run_dist_object_vector();
	return hpx::finalize();
}

int main(int argc, char *argv[]) { return hpx::init(argc, argv); }
