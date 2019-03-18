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

			void print() const {
				for (size_t i = 0; i < data_.size(); i++) {
					cout << data_[i] << " ";
				}
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

#define REGISTER_TEMPLATE_DIST_OBJECT_DECLARATION(type)                        \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
                                                                               \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
      dist_object::server::partition<type>::print_action,                    \
      HPX_PP_CAT(__dist_object_print_action_, type));                          \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
      dist_object::server::partition<type>::size_action,                     \
      HPX_PP_CAT(__dist_object_size_action_, type));                           \
  /**/

#define REGISTER_TEMPLATE_DIST_OBJECT(type)                                    \
  HPX_REGISTER_ACTION(                                                         \
      dist_object::server::partition<type>::print_action,                    \
      HPX_PP_CAT(__dist_object_print_action_, type));                          \
  HPX_REGISTER_ACTION(                                                         \
      dist_object::server::partition<type>::size_action,                     \
      HPX_PP_CAT(__dist_object_size_action_, type));                           \
  typedef ::hpx::components::component<                                        \
      dist_object::server::partition<type>>                                  \
      HPX_PP_CAT(__dist_object_, type);                                        \
  HPX_REGISTER_COMPONENT(HPX_PP_CAT(__dist_object_, type))                     \
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

		dist_object(data_type && data)
		: base_type(create_server(std::move(data)))
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

		void print() const {
			for (size_t i = 0; i < data_.size(); i++) {
				cout << data_[i] << " ";
			}
			cout << endl;
		}

		size_t size() {
			return data_.size();
		}
	};
}

void run_dist_object_matrix() {
	vector<int> a(10, 1);
	dist_object::server::partition<int> A(a);
	cout << (*A)[1] << endl;
	return;
}

int hpx_main() {
	run_dist_object_matrix();
	return hpx::finalize();
}

int main(int argc, char* argv[]) {
	return hpx::init(argc, argv);
}