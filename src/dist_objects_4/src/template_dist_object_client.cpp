// Copyright (c) 2019 Weile Wei
// Copyright (c) 2019 Maxwell Reeser
// Copyright (c) 2019 Hartmut Kaiser
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

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
#include <hpx/include/parallel_algorithm.hpp>
#include <hpx/lcos/barrier.hpp>
#include <hpx/parallel/algorithms/for_each.hpp>

#include <hpx/lcos/async.hpp>
#include <hpx/lcos/dataflow.hpp>
#include <hpx/lcos/when_all.hpp>

#include "template_dist_object.hpp"
#include <boost/range/irange.hpp>

#include <atomic>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

REGISTER_DIST_OBJECT_PART(int);

using myVectorInt = std::vector<int>;
REGISTER_DIST_OBJECT_PART(myVectorInt);
using myMatrixInt = std::vector<std::vector<int>>;
REGISTER_DIST_OBJECT_PART(myMatrixInt);

REGISTER_DIST_OBJECT_PART(double);
using myVectorDouble = std::vector<double>;
REGISTER_DIST_OBJECT_PART(myVectorDouble);
using myMatrixDouble = std::vector<std::vector<double>>;
REGISTER_DIST_OBJECT_PART(myMatrixDouble);
using myVectorDoubleConst = std::vector<double> const;
REGISTER_DIST_OBJECT_PART(myVectorDoubleConst);

using intRef = int &;
REGISTER_DIST_OBJECT_PART(intRef);
using myVectorIntRef = std::vector<int> &;
REGISTER_DIST_OBJECT_PART(myVectorIntRef);
using myVectorDoubleConstRef = std::vector<double> const &;
REGISTER_DIST_OBJECT_PART(myVectorDoubleConstRef);

void run_dist_object_int() {
  using dist_object::dist_object;
  // Construct a distrtibuted object of type int in all provided localities
  // User needs to provide the distributed object with a unique basename
  // and data for construction. The unique basename string enables HPX
  // register and retrive the distributed object.
  dist_object<int> dist_int("a_unique_name_string",
                            hpx::get_locality_id() + 42);
  // If there exists more than 2 (and include) localities, we are able to
  // asychronously fetch a future of a copy of the instance of this
  // distributed object associated with the given locality
  if (hpx::get_locality_id() >= 2) {
    assert(dist_int.fetch(1).get() == 43);
  }
}

void run_accumulation_reduce_to_locality0() {
  using dist_object::dist_object;
  int expect_res = 0, target_res = 0;
  size_t num_localities = hpx::find_all_localities().size();
  int cur_locality = hpx::get_locality_id();

  // declare a distributed object on every locality
  dist_object<int> dist_int("dist_int", cur_locality);

  // create a barrier and wait for the distributed object to be constructed in
  // all localities
  hpx::lcos::barrier wait_for_construction("wait_for_construction",
                                           num_localities, cur_locality);
  wait_for_construction.wait();

  if (cur_locality == 0) {
    // compute expect result
    // locality 0 gets all values
    for (int i = 0; i < num_localities; i++) {
      // fetch the distributed object from remote locality i
      expect_res += dist_int.fetch(i).get();
    }

    // compute target result
    // to verify the accumulation results
    for (int i = 0; i < num_localities; i++) {
      target_res += i;
    }

    assert(expect_res == target_res);
  }
}

void add_op(int &local, hpx::future<int> remote) { local += remote.get(); }

void run_accumulation_reduce_to_locality0_parallel() {
  using dist_object::dist_object;
  std::atomic<int> expect_res = 0;
  int target_res = 0;
  int num_localities = hpx::find_all_localities().size();
  int cur_locality = hpx::get_locality_id();

  // declare a distributed object on every locality
  dist_object<int> dist_int("dist_int", cur_locality);

  // create a barrier and wait for the distributed object to be constructed in
  // all localities
  hpx::lcos::barrier wait_for_construction("wait_for_construction",
                                           num_localities, cur_locality);
  wait_for_construction.wait();

  if (cur_locality == 0 && num_localities>=2) {
    using hpx::parallel::for_each;
    using hpx::parallel::execution::par;
    auto range = boost::irange(1, num_localities);
    // compute expect result in parallel
    // locality 0 fetchs all values
    for_each(par, std::begin(range), std::end(range),
             [&](std::uint64_t b) { expect_res += dist_int.fetch(b).get(); });
    hpx::wait_all();
    // compute target result
    // to verify the accumulation results
    for (int i = 0; i < num_localities; i++) {
      target_res += i;
    }
    assert(expect_res == target_res);
  }
}

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
  hpx::lcos::barrier b_dist_vector("b_dist_vector",
                                   hpx::find_all_localities().size(),
                                   hpx::get_locality_id());
  b_dist_vector.wait();

  // perform element-wise addition between dist_objects
  for (int i = 0; i < len; i++) {
    (*RES)[i] = (*LHS)[i] + (*RHS)[i];
  }

  for (int i = 0; i < len; i++) {
    res[i] = lhs[i] + rhs[i];
  }

  assert((*RES) == res);
  assert(RES->size() == len);
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
  assert(RES->size() == rows);
  hpx::lcos::barrier b_dist_matrix("b_dist_matrix",
                                   hpx::find_all_localities().size(),
                                   hpx::get_locality_id());
  b_dist_matrix.wait();

  // test fetch function when 2 or more localities provided
  if (hpx::find_all_localities().size() > 1) {
    if (hpx::get_locality_id() == 0) {
      hpx::future<myMatrixDouble> RES_first = RES.fetch(1);
      assert(RES_first.get()[0][0] == 86);
    } else {
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

  dist_object::dist_object<myMatrixDouble, c_t::All_to_All> LHS("m1", lhs);
  dist_object::dist_object<myMatrixDouble, c_t::All_to_All> RHS("m2", rhs);
  dist_object::dist_object<myMatrixDouble, c_t::All_to_All> RES("m3", res);

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      (*RES)[i][j] = (*LHS)[i][j] + (*RHS)[i][j];
      res[i][j] = lhs[i][j] + rhs[i][j];
    }
  }
  assert((*RES) == res);
  assert(RES->size() == rows);

  hpx::lcos::barrier b_dist_matrix_2("b_dist_matrix_2",
                                     hpx::find_all_localities().size(),
                                     hpx::get_locality_id());
  b_dist_matrix_2.wait();

  // test fetch function when 2 or more localities provided
  if (hpx::find_all_localities().size() > 1) {
    if (hpx::get_locality_id() == 0) {
      hpx::future<myMatrixDouble> RES_first = RES.fetch(1);
      assert(RES_first.get()[0][0] == 86);
    } else {
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

  dist_object::dist_object<myMatrixInt, c_t::Meta_Object> M1("M1_meta", m1);
  dist_object::dist_object<myMatrixInt, c_t::Meta_Object> M2("M2_meta", m2);
  dist_object::dist_object<myMatrixInt, c_t::Meta_Object> M3("M3_meta", m3);

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

  hpx::future<std::vector<std::vector<int>>> k = M3.fetch(
      (hpx::get_locality_id() + 1) % hpx::find_all_localities().size());
  std::cout
      << "The value of other partition's first element (with meta_object) is "
      << k.get()[0][0] << std::endl;
  assert((*M3) == m3);
  assert(M3->size() == rows);
}

void run_dist_object_ref() {
  size_t n = 10;
  int val = 2;
  int pos = 3;
  assert(pos < n);

  int val_update = 42;
  myVectorInt vec1(n, val);
  dist_object::dist_object<myVectorInt &> dist_vec("vec1", vec1);

  vec1[2] = val_update;

  assert((*dist_vec)[2] == val_update);
  assert(dist_vec->size() == n);
}

void run_dist_object_const_ref() {
  size_t n = 10;
  int val = 2;
  int pos = 3;
  assert(pos < n);

  int val_update = 42;
  myVectorDoubleConst vec1(n, val);
  dist_object::dist_object<myVectorDoubleConstRef> dist_vec("vec1", vec1);
}


void run_dist_object_matrix_mo_loc_list(std::vector<size_t> locs) 
{
	auto loc_it = std::find(locs.begin(), locs.end(), hpx::get_locality_id());
	if ( loc_it	== locs.end())
		return;
	size_t here = hpx::get_locality_id();
	std::size_t barrier_rank = std::distance(locs.begin(), loc_it);
	int val = 42 + static_cast<int>(hpx::get_locality_id());
	int rows = 5, cols = 5;

	std::vector<std::vector<int>> m1(rows, std::vector<int>(cols, val));
	std::vector<std::vector<int>> m2(rows, std::vector<int>(cols, val));
	std::vector<std::vector<int>> m3(rows, std::vector<int>(cols, 0));

	//std::vector<std::size_t> locs(hpx::find_all_localities().size()-1);
	//std::iota(locs.begin(), locs.end(), 0);

	typedef dist_object::construction_type c_t;
	std::string sub_basename = std::to_string(locs[0]) + std::to_string(locs[1]);
	dist_object::dist_object<std::vector<std::vector<int>>, c_t::Meta_Object> M1(
		"M1_meta_loc_list/"+sub_basename, m1, locs);
	dist_object::dist_object<std::vector<std::vector<int>>, c_t::Meta_Object> M2(
		"M2_meta_loc_list"+sub_basename, m2, locs);
	dist_object::dist_object<std::vector<std::vector<int>>, c_t::Meta_Object> M3(
		"M3_meta_loc_list"+sub_basename, m3, locs);


	size_t here_idx = std::find(locs.begin(), locs.end(), here) - locs.begin();

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
	std::string barrier_name = "/loc_list/barrier" + std::to_string(locs[0]) + std::to_string(locs[1]);
	hpx::lcos::barrier b(barrier_name, locs, hpx::get_locality_id());
	b.wait();

	hpx::future<std::vector<std::vector<int>>> k = M3.fetch( 
		locs[(here_idx+1)%locs.size()] );
	std::cout << "The value of first partition's first element "
		<< "(with meta_object and loc list) is " << k.get()[0][0] << std::endl;
	assert((*M3) == m3);
}


void run_dist_object_matrix_mul() {
  int cols = 5; // Decide how big the matrix should be

  size_t num_locs = hpx::find_all_localities().size();
  std::vector<std::pair<size_t, size_t>> ranges(num_locs);

  // Create a list of row ranges for each partition
  size_t start = 0;
  size_t diff = (int)std::ceil((double)cols / ((double)num_locs));
  for (int i = 0; i < num_locs; i++) {
    size_t second = min(cols, start + diff);
    ranges[i] = std::make_pair(start, second);
    start += diff;
  }

  // Create our data, stored in all_data's. This way we can check for validity
  // without using anything distributed. The seed being a constant is needed
  // in order for all nodes to generate the same data
  size_t here = hpx::get_locality_id();
  size_t local_rows = ranges[here].second - ranges[here].first;
  std::vector<std::vector<std::vector<int>>> all_data_m1(
      hpx::find_all_localities().size());

  std::srand(123456);

  for (int i = 0; i < all_data_m1.size(); i++) {
    size_t tmp_num_rows = ranges[i].second - ranges[i].first;
    all_data_m1[i] =
        std::vector<std::vector<int>>(tmp_num_rows, std::vector<int>(cols, 0));
    for (int j = 0; j < tmp_num_rows; j++) {
      for (int k = 0; k < cols; k++) {
        all_data_m1[i][j][k] = std::rand();
      }
    }
  }

  std::vector<std::vector<std::vector<int>>> all_data_m2(
      hpx::find_all_localities().size());

  std::srand(7891011);

  for (int i = 0; i < all_data_m2.size(); i++) {
    size_t tmp_num_rows = ranges[i].second - ranges[i].first;
    all_data_m2[i] =
        std::vector<std::vector<int>>(tmp_num_rows, std::vector<int>(cols, 0));
    for (int j = 0; j < tmp_num_rows; j++) {
      for (int k = 0; k < cols; k++) {
        all_data_m2[i][j][k] = std::rand();
      }
    }
  }

  std::vector<std::vector<int>> here_data_m3(local_rows,
                                             std::vector<int>(cols, 0));

  typedef dist_object::construction_type c_t;

  dist_object::dist_object<myMatrixInt, c_t::Meta_Object> M1("M1_meta_mat_mul",
	  all_data_m1[here]);
  dist_object::dist_object<myMatrixInt, c_t::Meta_Object> M2("M2_meta_mat_mul",
	  all_data_m2[here]);
  dist_object::dist_object<myMatrixInt, c_t::Meta_Object> M3("M3_meta_mat_mul",
	  here_data_m3);

  // Actual matrix multiplication. For non-local values, get the data
  // and then use it, for local, just use the local data without doing
  // a fetch to get it
  size_t num_before_me = here;
  size_t num_after_me = hpx::find_all_localities().size() - 1 - here;
  int other_val;
  for (int p = 0; p < num_before_me; p++) {
    std::vector<std::vector<int>> non_local = M2.fetch(p).get();
    std::size_t non_local_rows = ranges[p].second - ranges[p].first;
    if (p == 0)
      other_val = non_local[0][0];
    for (int i = 0; i < local_rows; i++) {
      for (int j = ranges[p].first; j < ranges[p].second; j++) {
        for (int k = 0; k < cols; k++) {
          (*M3)[i][j] += (*M1)[i][k] * non_local[j - ranges[p].first][k];
          here_data_m3[i][j] +=
              all_data_m1[here][i][k] * all_data_m2[p][j - ranges[p].first][k];
        }
      }
    }
  }
  for (int i = 0; i < local_rows; i++) {
    for (int j = ranges[here].first; j < ranges[here].second; j++) {
      for (int k = 0; k < cols; k++) {
        (*M3)[i][j] += (*M1)[i][k] * (*M2)[j - ranges[here].first][k];
        here_data_m3[i][j] += all_data_m1[here][i][k] *
                              all_data_m2[here][j - ranges[here].first][k];
      }
    }
  }
  for (int p = here + 1; p < num_locs; p++) {
    std::vector<std::vector<int>> non_local = M2.fetch(p).get();
    std::size_t non_local_rows = ranges[p].second - ranges[p].first;
    if (p == here + 1)
      other_val = non_local[0][0];
    for (int i = 0; i < local_rows; i++) {
      for (int j = ranges[p].first; j < ranges[p].second; j++) {
        for (int k = 0; k < cols; k++) {
          (*M3)[i][j] += (*M1)[i][k] * non_local[j - ranges[p].first][k];
          here_data_m3[i][j] +=
              all_data_m1[here][i][k] * all_data_m2[p][j - ranges[p].first][k];
        }
      }
    }
  }
  std::vector<std::vector<int>> tmp = *M3;
  assert((*M3) == here_data_m3);
}

int hpx_main() {
  std::cout << "Hello world from locality " << hpx::get_locality_id()
            << std::endl;
  run_dist_object_int();
  run_accumulation_reduce_to_locality0_parallel();
  run_accumulation_reduce_to_locality0();
  run_dist_object_vector();
  run_dist_object_matrix();
  run_dist_object_matrix_all_to_all();
  run_dist_object_matrix_mo();
  run_dist_object_matrix_mul();
  run_dist_object_ref();
  run_dist_object_const_ref();
  std::vector<size_t> locs0{ 0,1 };
  run_dist_object_matrix_mo_loc_list(locs0);
  std::vector<size_t> locs1{ 0,2 };
  run_dist_object_matrix_mo_loc_list(locs1);
  std::vector<size_t> locs2{ 1,2 };
  run_dist_object_matrix_mo_loc_list(locs2);
  return hpx::finalize();
}

int main(int argc, char *argv[]) { std::cout << "Before hpx::init" << std::endl; return hpx::init(argc, argv); }
