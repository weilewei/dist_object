//  Copyright (c) 2019 Weile Wei
//  Copyright (c) Maxwell Resser
//  Copyright (c) 2019 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/include/components.hpp>
#include <hpx/include/actions.hpp>

#include <hpx/util/detail/pp/cat.hpp>

#include <hpx/hpx.hpp>
#include <hpx/runtime/components/component_factory.hpp>
#include <hpx/runtime/get_ptr.hpp>

#include <vector>
#include <iostream>
#include <string>

using namespace std;

namespace dist_object {
	namespace server {
		template <typename T>
		class partition
			: public hpx::components::locking_hook<
			hpx::components::component_base<partition<T> > >
		{
		public:
			typedef std::vector<T> data_type;
			partition() {}

			partition(data_type const& data) : data_(data) {}

			partition(data_type && data) : data_(data) {}

			partition operator+(partition b) {
				assert(data_.size() == (*b).size());
				data_type tmp;
				tmp.resize(data_.size());
				for (size_t i = 0; i < data_.size(); i++) {
					tmp[i] = data_[i] + (*b)[i];
				}
				partition<T> res(std::move(tmp));
				return res;
			}

			void print() {
				for (size_t i = 0; i < data_.size(); i++) {
					cout << data_[i] << " ";
				}
				//cout << "hello";
				cout << endl;
			}

			size_t size() {
				return data_.size();
			}

			data_type &operator*() { return data_; }
			HPX_DEFINE_COMPONENT_ACTION(partition, print);
			HPX_DEFINE_COMPONENT_ACTION(partition, size);

		private:
			data_type data_;
		};
	}
}

#define REGISTER_PARTITION_DECLARATION(type)                        \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
                                                                               \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
      dist_object::server::partition<type>::print_action,                    \
      HPX_PP_CAT(__partition_print_action_, type));                          \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
      dist_object::server::partition<type>::size_action,                     \
      HPX_PP_CAT(__partition_size_action_, type));                           \
  /**/

#define REGISTER_PARTITION(type)                                    \
  HPX_REGISTER_ACTION(                                                         \
      dist_object::server::partition<type>::print_action,                    \
      HPX_PP_CAT(__partition_print_action_, type));                          \
  HPX_REGISTER_ACTION(                                                         \
      dist_object::server::partition<type>::size_action,                     \
      HPX_PP_CAT(__partition_size_action_, type));                           \
  typedef ::hpx::components::component<                                        \
      dist_object::server::partition<type>>                                  \
      HPX_PP_CAT(__partition_, type);                                        \
  HPX_REGISTER_COMPONENT(HPX_PP_CAT(__partition_, type))                     \
  /**/

HPX_REGISTER_COMPONENT_MODULE(); // not for exe

namespace dist_object {
	template<typename T>
	class dist_object
		: hpx::components::client_base <
		dist_object<T>, server::partition<T>
		> 
	{
		typedef hpx::components::client_base<
			dist_object<T>, server::partition<T>
		> base_type;

		typedef typename server::partition<T>::data_type
			data_type;
	private:
		template <typename Arg>
		static hpx::future<hpx::id_type> create_server(Arg && value)
		{
			return hpx::new_<server::partition<T>>(hpx::find_here(), std::forward<Arg>(value));
		}

	public:
		dist_object() {}

		dist_object(data_type const& data)
			: base_type(create_server(data))
		{}

		dist_object(data_type && data)
		: base_type(create_server(std::move(data)))
		{}

		dist_object(hpx::future<hpx::id_type> &&id)
			: base_type(std::move(id))
		{}

		dist_object(hpx::id_type &&id)
			: base_type(std::move(id))
		{}

		dist_object operator+(dist_object b) {
			assert(data_.size() == (*b).size());
			data_type tmp;
			tmp.resize(data_.size());
			for (size_t i = 0; i < data_.size(); i++) {
				tmp[i] = data_[i] + (*b)[i];
			}
			dist_object<T> res(tmp);
			return res;
		}

		void print() {
			HPX_ASSERT(this->get_id());
			ensure_ptr();
			for (size_t i = 0; i < *ptr.size(); i++) {
				cout << **ptr[i] << " ";
			}
			cout << endl;
		}

		size_t size() {
			HPX_ASSERT(this->get_id());
			ensure_ptr();
			//cout << "hello\n";
			return (**ptr).size();
		}

		data_type const& operator*() const
		{
			HPX_ASSERT(this->get_id());
			ensure_ptr();
			return **ptr;
		}

		data_type &operator*() { 
			HPX_ASSERT(this->get_id());
			ensure_ptr();
			return **ptr;
		}

	private:
		mutable std::shared_ptr<server::partition<T>> ptr;
		void ensure_ptr() const {
			if (!ptr) {
				ptr = hpx::get_ptr<server::partition<T>>(hpx::launch::sync, get_id());
			}
		}
	};
}

namespace dist_object {
	template<typename T>
	class dist_object_config_data {
	public:
		typedef typename server::partition<T>::data_type
			data_type;

		typedef dist_object::dist_object<T> client_type;
		typedef vector<shared_ptr<client_type>> client_list_type;

		dist_object_config_data()
			: values_(client_list_type())
		{
			values_.resize(1);
		}

		dist_object_config_data(size_t num_partitions) 
			: values_(client_list_type()) 
		{
			values_.resize(num_partitions);
		}

		dist_object_config_data(data_type &&values) 
			: values_(std::move(values)) 
		{}

		dist_object_config_data(data_type const &values) 
			: values_(values) 
		{}

		dist_object_config_data(data_type const &val, size_t num_partitions) {
			size_t num_partitions_tmp = num_partitions;
			size_t slice_begin = 0;
			size_t last_idx = val.size();
			while (num_partitions_tmp > 0) {
				size_t block_size = (size_t)ceil(double(last_idx) / double(num_partitions_tmp));
				size_t slice_end = slice_begin + block_size;
				data_type partition_data = slice(val, slice_begin, slice_end);
				client_type partition(partition_data);
				values_.push_back(make_shared<client_type>(partition));
				slice_begin = slice_end;
				num_partitions_tmp--;
				last_idx -= block_size;
			}
		}

		data_type slice(data_type const &v, size_t m, size_t n)
		{
			auto first = v.cbegin() + m;
			auto last = v.cbegin() + n;

			data_type vec(first, last);
			return vec;
		}

		//dist_object_config_data operator+(dist_object_config_data b) {
		//	assert(gather_num_partitions() == b.gather_num_partitions());
		//	dist_object result(gather_num_partitions());
		//	for (size_t i = 0; i < gather_num_partitions(); i++) {
		//		client_type tmp = *(values_[i]) + *(*b)[i];
		//		(*result)[i] = make_shared<client_type>(tmp);
		//	}
		//	return result;
		//}

		void print() const {
			for (size_t i = 0; i < values_.size(); i++) {
				cout << "Part " << i << endl;
				for (size_t j = 0; j < (*(values_[i])).size(); j++) {
					cout << (*(*(values_[i])))[j] << " ";
				}
				cout << endl;
			}
		}

	private:
		client_list_type values_;
		size_t num_partitions_; // number of partition instances
	};
}

REGISTER_PARTITION(int);
using myVectorInt = std::vector<int>;
//REGISTER_PARTITION(myVectorInt);
void run_dist_object_vector() {
	//typedef typename dist_object::server::partition<int> dist_object_type;
	//typedef typename dist_object_type::data_type data_type;

	//std::vector<hpx::id_type> localities = hpx::find_all_localities();

	//vector<int> a(10, 1);
	//dist_object::dist_object<int> A(a);
	////dist_object::server::partition<int> A(a);
	//cout << (*A)[1] << endl;
	////dist_object::config_data<int> obj();

	vector<int> a(10, 2);
	dist_object::dist_object_config_data<int> test(a, 2);
	test.print();

	return;
}

int hpx_main() {
	run_dist_object_vector();
	return hpx::finalize();
}

int main(int argc, char* argv[]) {
	return hpx::init(argc, argv);
}