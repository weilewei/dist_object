//  Copyright (c) 2019 Weile Wei
//  Copyright (c) Maxwell Resser
//  Copyright (c) 2019 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0325PM)
#define HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0325PM

#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/util/detail/pp/cat.hpp>

#include <vector>

namespace dist_object {
	namespace server {
		template <typename T>
		class partition : public hpx::components::locking_hook<
			hpx::components::component_base<partition<T>>> {
		public:
			typedef std::vector<T> data_type;
			partition() {}

			partition(data_type const &data) : data_(data) {}

			partition(data_type &&data) : data_(std::move(data)) {}

			size_t size() { return data_.size(); }

			data_type &operator*() { return data_; }

			data_type const &operator*() const { return data_;}

			data_type const* operator->() const
			{
				return &data_;
			}

			data_type* operator->()
			{
				return &data_;
			}

			HPX_DEFINE_COMPONENT_ACTION(partition, size);

		private:
			data_type data_;
		};
	}
}

#define REGISTER_PARTITION_DECLARATION(type)                                   \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
                                                                               \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
      dist_object::server::partition<type>::size_action,                     \
      HPX_PP_CAT(__partition_size_action_, type));                             \
  /**/

#define REGISTER_PARTITION(type)                                               \
  HPX_REGISTER_ACTION(dist_object::server::partition<type>::size_action,       \
                      HPX_PP_CAT(__partition_size_action_, type));             \
  typedef ::hpx::components::component<dist_object::server::partition<type>>   \
      HPX_PP_CAT(__partition_, type);                                          \
  HPX_REGISTER_COMPONENT(HPX_PP_CAT(__partition_, type))                       \
  /**/
#endif
