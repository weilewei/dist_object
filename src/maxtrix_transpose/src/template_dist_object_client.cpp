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

#include "template_dist_object.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

REGISTER_PARTITION(int);
using myVectorInt = std::vector<int>;
REGISTER_PARTITION(myVectorInt);

void run_matrix_transposition() {
	return;
}


int hpx_main() {
	run_matrix_transposition();
	std::cout << "Hello world from locality " << hpx::get_locality_id() << std::endl;
	return hpx::finalize();
}

int main(int argc, char *argv[]) { return hpx::init(argc, argv); }
