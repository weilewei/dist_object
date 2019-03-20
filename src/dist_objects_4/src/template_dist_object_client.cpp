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
	int here_ = static_cast<int> (hpx::get_locality_id());
	std::vector<int> a(10, here_);
	std::vector<int> b(10, here_);

	dist_object::dist_object<int> A(hpx::find_here(), a);
	dist_object::dist_object<int> B(hpx::find_here(), b);
	auto C = A + B;
	C.print();
	//char const* msg = "hello world from locality {1}\n";

	//hpx::util::format_to(hpx::cout, msg, hpx::get_locality_id())
	//	<< hpx::flush;
	return;
}

int hpx_main() {
	run_dist_object_vector();
	return hpx::finalize();
}

int main(int argc, char* argv[]) {
	return hpx::init(argc, argv);
}
