//  Copyright (c) 2019 Weile Wei
//  Copyright (c) 2019 Maxwell Reeser
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

void run_dist_object_vector() {
	// define vector based on the locality that it is running
	int here_ = static_cast<int>(hpx::get_locality_id());
	int len = 10;
	std::vector<int> a(len, here_);
	std::vector<int> b(len, here_);
	std::vector<int> c(len, 0);

	// construct int type dist_objects to be used later
	dist_object::dist_object<int> A("a", a);
	dist_object::dist_object<int> B("b", b);
	dist_object::dist_object<int> C("c", c);

	// perform element-wise addition between dist_objects
	for (int i = 0; i < len; i++) {
		(*C)[i] = (*A)[i] + (*B)[i];
	}

	for (int i = 0; i < len; i++) {
		c[i] = a[i] + b[i];
	}
	//A.print();
	assert((*C) == c);
}

void run_dist_object_matrix() {
	int val = 42 + static_cast<int>(hpx::get_locality_id());
	int rows = 5, cols = 5;

	std::vector<std::vector<int>> m1(rows, std::vector<int>(cols, val));
	std::vector<std::vector<int>> m2(rows, std::vector<int>(cols, val));
	std::vector<std::vector<int>> m3(rows, std::vector<int>(cols, 0));

	dist_object::dist_object<std::vector<int>> M1("m1", m1);
	dist_object::dist_object<std::vector<int>> M2("m2", m2);
	dist_object::dist_object<std::vector<int>> M3("m3", m3);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			(*M3)[i][j] = (*M1)[i][j] + (*M2)[i][j];
			m3[i][j] = m1[i][j] + m2[i][j];
		}
	}
	assert((*M3) == m3);

	hpx::lcos::barrier b("a global barrier", hpx::find_all_localities().size(), hpx::get_locality_id());
	b.wait();
	
	if (hpx::get_locality_id() == 0) {
		hpx::future<std::vector<std::vector<int>>> k = M3.fetch(1);
		std::cout << "The value of other partition's first element is " << k.get()[0][0] << std::endl;
	}
	else {
		hpx::future<std::vector<std::vector<int>>> k = M3.fetch(0);
		std::cout << "The value of other partition's first element is " << k.get()[0][0] << std::endl;
	}
	
}

void run_dist_object_matrix_all_to_all() {
	int val = 42 + static_cast<int>(hpx::get_locality_id());
	int rows = 5, cols = 5;

	std::vector<std::vector<int>> m1(rows, std::vector<int>(cols, val));
	std::vector<std::vector<int>> m2(rows, std::vector<int>(cols, val));
	std::vector<std::vector<int>> m3(rows, std::vector<int>(cols, 0));

	typedef dist_object::construction_type c_t;

	dist_object::dist_object<std::vector<int>> M1("m1", m1, c_t::All_to_All);
	dist_object::dist_object<std::vector<int>> M2("m2", m2, c_t::All_to_All);
	dist_object::dist_object<std::vector<int>> M3("m3", m3, c_t::All_to_All);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			(*M3)[i][j] = (*M1)[i][j] + (*M2)[i][j];
			m3[i][j] = m1[i][j] + m2[i][j];
		}
	}
	assert((*M3) == m3);

	hpx::lcos::barrier b("a global barrier", hpx::find_all_localities().size(), hpx::get_locality_id());
	b.wait();

	if (hpx::get_locality_id() == 0) {
		hpx::future<std::vector<std::vector<int>>> k = M3.fetch(1);
		std::cout << "The value of other partition's first element is " << k.get()[0][0] << std::endl;
	}
	else {
		hpx::future<std::vector<std::vector<int>>> k = M3.fetch(0);
		std::cout << "The value of other partition's first element is " << k.get()[0][0] << std::endl;
	}

}


void run_dist_object_matrix_mo() {
	int val = 42 + static_cast<int>(hpx::get_locality_id());
	int rows = 5, cols = 5;

	std::vector<std::vector<int>> m1(rows, std::vector<int>(cols, val));
	std::vector<std::vector<int>> m2(rows, std::vector<int>(cols, val));
	std::vector<std::vector<int>> m3(rows, std::vector<int>(cols, 0));

	typedef dist_object::construction_type c_t;

	dist_object::dist_object<std::vector<int>> M1("M1_meta", m1, c_t::Meta_Object);
	dist_object::dist_object<std::vector<int>> M2("M2_meta", m2, c_t::Meta_Object);
	dist_object::dist_object<std::vector<int>> M3("M3_meta", m3, c_t::Meta_Object);

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
	hpx::lcos::barrier b("/meta/barrier", hpx::find_all_localities().size());
	b.wait();

	hpx::future<std::vector<std::vector<int>>> k = M3.fetch((hpx::get_locality_id() + 1) % hpx::find_all_localities().size());
	std::cout << "The value of other partition's first element (with meta_object) is " << k.get()[0][0] << std::endl;
	assert((*M3) == m3);
}

int hpx_main() {
	run_dist_object_vector();
	run_dist_object_matrix();
	run_dist_object_matrix_all_to_all();
	run_dist_object_matrix_mo();
	std::cout << "Hello world from locality " << hpx::get_locality_id() << std::endl;
	return hpx::finalize();
}

int main(int argc, char *argv[]) { return hpx::init(argc, argv); }
