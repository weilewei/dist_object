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


using myVectorInt = std::vector<int>;
REGISTER_PARTITION(myVectorInt);
using myMatrixInt = std::vector<std::vector<int>>;
REGISTER_PARTITION(myMatrixInt);

REGISTER_PARTITION(double);
using myVectorDouble = std::vector<double>;
REGISTER_PARTITION(myVectorDouble);
using myMatrixDouble = std::vector<std::vector<double>>;
REGISTER_PARTITION(myMatrixDouble);


using intRef = int&;
REGISTER_PARTITION(intRef);
using myVectorIntRef = std::vector<int>&;
REGISTER_PARTITION(myVectorIntRef);


void run_dist_object_vector() {
	// define vector based on the locality that it is running
	int here_ = static_cast<int>(hpx::get_locality_id());
	int len = 10;

	// prepare vector data
	std::vector<int> lhs(len, here_);
	std::vector<int> rhs(len, here_);
	std::vector<int> res(len, 0);

	// construct a dist_object with vector<int> type
	dist_object::dist_object<std::vector<int>> LHS("lhs_vec", lhs);
	dist_object::dist_object<std::vector<int>> RHS("rhs_vec", rhs);
	dist_object::dist_object<std::vector<int>> RES("res_vec", res);

	// construct a gobal barrier
	hpx::lcos::barrier b_dist_vector("b_dist_vector", hpx::find_all_localities().size(), hpx::get_locality_id());
	b_dist_vector.wait();

	// perform element-wise addition between dist_objects
	for (int i = 0; i < len; i++) {
		(*RES)[i] = (*LHS)[i] + (*RHS)[i];
	}

	for (int i = 0; i < len; i++) {
		res[i] = lhs[i] + rhs[i];
	}

	assert((*RES) == res);
}

// element-wise addition for vector<vector<double>> for dist_object
void run_dist_object_matrix() {
	double val = 42.0 + static_cast<double>(hpx::get_locality_id());
	int rows = 5, cols = 5;

	myMatrixDouble lhs(rows, std::vector<double>(cols, val));
	myMatrixDouble rhs(rows, std::vector<double>(cols, val));
	myMatrixDouble res(rows, std::vector<double>(cols, 0));

	dist_object::dist_object<myMatrixDouble> LHS("m1", lhs);
	dist_object::dist_object<myMatrixDouble> RHS("m2", rhs);
	dist_object::dist_object<myMatrixDouble> RES("m3", res);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			(*RES)[i][j] = (*LHS)[i][j] + (*RHS)[i][j];
			res[i][j] = lhs[i][j] + rhs[i][j];
		}
	}
	assert((*RES) == res);

	hpx::lcos::barrier b_dist_matrix("b_dist_matrix", hpx::find_all_localities().size(), hpx::get_locality_id());
	b_dist_matrix.wait();
	
	// test fetch function when 2 or more localities provided
	if (hpx::find_all_localities().size() > 1) {
		if (hpx::get_locality_id() == 0) {
			hpx::future<myMatrixDouble> RES_first = RES.fetch(1);
			assert(RES_first.get()[0][0] == 86);
		}
		else {
			hpx::future<myMatrixDouble> RES_first = RES.fetch(0);
			assert(RES_first.get()[0][0] == 84);
		}
	}	
}

void run_dist_object_matrix_all_to_all() {
	double val = 42.0 + static_cast<double>(hpx::get_locality_id());
	int rows = 5, cols = 5;

	myMatrixDouble lhs(rows, std::vector<double>(cols, val));
	myMatrixDouble rhs(rows, std::vector<double>(cols, val));
	myMatrixDouble res(rows, std::vector<double>(cols, 0));

	typedef dist_object::construction_type c_t;

	dist_object::dist_object<myMatrixDouble> LHS("m1", lhs, c_t::All_to_All);
	dist_object::dist_object<myMatrixDouble> RHS("m2", rhs, c_t::All_to_All);
	dist_object::dist_object<myMatrixDouble> RES("m3", res, c_t::All_to_All);

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			(*RES)[i][j] = (*LHS)[i][j] + (*RHS)[i][j];
			res[i][j] = lhs[i][j] + rhs[i][j];
		}
	}
	assert((*RES) == res);

	hpx::lcos::barrier b_dist_matrix("b_dist_matrix", hpx::find_all_localities().size(), hpx::get_locality_id());
	b_dist_matrix.wait();

	// test fetch function when 2 or more localities provided
	if (hpx::find_all_localities().size() > 1) {
		if (hpx::get_locality_id() == 0) {
			hpx::future<myMatrixDouble> RES_first = RES.fetch(1);
			assert(RES_first.get()[0][0] == 86);
		}
		else {
			hpx::future<myMatrixDouble> RES_first = RES.fetch(0);
			assert(RES_first.get()[0][0] == 84);
		}
	}
}


void run_dist_object_matrix_mo() {
	int val = 42 + static_cast<int>(hpx::get_locality_id());
	int rows = 5, cols = 5;

	myMatrixInt m1(rows, std::vector<int>(cols, val));
	myMatrixInt m2(rows, std::vector<int>(cols, val));
	myMatrixInt m3(rows, std::vector<int>(cols, 0));

	typedef dist_object::construction_type c_t;

	dist_object::dist_object<myMatrixInt> M1("M1_meta", m1, c_t::Meta_Object);
	dist_object::dist_object<myMatrixInt> M2("M2_meta", m2, c_t::Meta_Object);
	dist_object::dist_object<myMatrixInt> M3("M3_meta", m3, c_t::Meta_Object);

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

void run_dist_object_ref() {
	size_t n = 10;
	int val = 2;
	int pos = 3;
	assert(pos < n);

	int val_update = 42;
	myVectorInt vec1(n, val);
	dist_object::dist_object<myVectorInt&> dist_vec("vec1", vec1);

	vec1[2] = val_update;
	
	assert((*dist_vec)[2] == val_update);
	assert((*dist_vec).size() == n);
}

int hpx_main() {
	run_dist_object_vector();
	run_dist_object_matrix();
	run_dist_object_matrix_all_to_all();
	run_dist_object_matrix_mo();
	run_dist_object_ref();
	std::cout << "Hello world from locality " << hpx::get_locality_id() << std::endl;
	return hpx::finalize();
}

int main(int argc, char *argv[]) { return hpx::init(argc, argv); }
