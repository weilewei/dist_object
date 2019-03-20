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

#include <algorithm>
#include <vector>
#include <iostream>
#include <string>
#include <hpx/include/iostreams.hpp>
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
					hpx::cout << data_[i] << " ";
				}
				char const* msg = "hello world from locality {1}\n";

				hpx::util::format_to(hpx::cout, msg, hpx::get_locality_id())
					<< hpx::flush;
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

		typedef typename server::partition<T>::data_type data_type;
	private:
		template <typename Arg>
		static hpx::future<hpx::id_type> create_server(hpx::id_type where, Arg && value)
		{
			return hpx::new_<server::partition<T>>(where, std::forward<Arg>(value));
		}

	public:
		dist_object() {}

		dist_object(hpx::id_type where, data_type const& data)
			: base_type(create_server(where, data))
		{}

		dist_object(hpx::id_type where, data_type && data)
		: base_type(create_server(where, std::move(data)))
		{}

		dist_object(hpx::future<hpx::id_type> &&id)
			: base_type(std::move(id))
		{}

		dist_object(hpx::id_type &&id)
			: base_type(std::move(id))
		{}

		dist_object operator+(dist_object b) {
			assert(this->size() == (*b).size());
			data_type tmp;
			tmp.resize(this->size());
			for (size_t i = 0; i < this->size(); i++) {
				tmp[i] = (**ptr)[i] + (*b)[i];
			}
			dist_object<T> res(hpx::find_here(), tmp);
			return res;
		}

		void print() {
			HPX_ASSERT(this->get_id());
			ensure_ptr();
			(*ptr).print();
		}

		size_t size() {
			HPX_ASSERT(this->get_id());
			ensure_ptr();
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

REGISTER_PARTITION(int);
using myVectorInt = std::vector<int>;

void run_dist_object_vector() {
	int here_ = static_cast<int> (hpx::get_locality_id());
	vector<int> a(10, here_);
	vector<int> b(10, here_);
	
	dist_object::dist_object<int> A(hpx::find_here(), a);
	dist_object::dist_object<int> B(hpx::find_here(), b);
	auto C = A + B;
	C.print();
	char const* msg = "hello world from locality {1}\n";

	hpx::util::format_to(hpx::cout, msg, hpx::get_locality_id())
		<< hpx::flush;
	return;
}

int hpx_main() {
	run_dist_object_vector();
	return hpx::finalize();
}

int main(int argc, char* argv[]) {
	return hpx::init(argc, argv);
}