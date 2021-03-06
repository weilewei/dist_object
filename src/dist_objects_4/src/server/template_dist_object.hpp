// Copyright (c) 2019 Weile Wei
// Copyright (c) 2019 Maxwell Reeser
// Copyright (c) 2019 Hartmut Kaiser
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0325PM)
#define HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0325PM

#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/util/detail/pp/cat.hpp>

#include <vector>

// Dist_object server maintains the local data for a given instance of
// dist_object, and responds to non-local requests for its data
namespace dist_object {
namespace server {
template <typename T>
class dist_object_part
    : public hpx::components::locking_hook<
          hpx::components::component_base<dist_object_part<T>>> {
public:
  typedef T data_type;
  dist_object_part() {}

  dist_object_part(data_type const &data) : data_(data) {}

  dist_object_part(data_type &&data) : data_(std::move(data)) {}

  data_type &operator*() { return data_; }

  data_type const &operator*() const { return data_; }

  data_type const *operator->() const { return &data_; }

  data_type *operator->() { return &data_; }

  data_type fetch() const { return data_; }

  HPX_DEFINE_COMPONENT_ACTION(dist_object_part, fetch);

private:
  data_type data_;
};

template <typename T>
class dist_object_part<T &>
    : public hpx::components::locking_hook<
          hpx::components::component_base<dist_object_part<T &>>> {
public:
  typedef T &data_type;
  dist_object_part() {}

  dist_object_part(data_type data) : data_(data) {}

  // dist_object_part(T const &data) : data_(data) {}

  data_type operator*() { return data_; }

  data_type const operator*() const { return data_; }

  T const *operator->() const { return data_; }

  T *operator->() { return data_; }

  T fetch() const { return data_; }

  HPX_DEFINE_COMPONENT_ACTION(dist_object_part, fetch);

private:
  data_type data_;
};
} // namespace server
} // namespace dist_object

#define REGISTER_DIST_OBJECT_PART_DECLARATION(type)                           \
  HPX_REGISTER_ACTION_DECLARATION(                                            \
      dist_object::server::dist_object_part<type>::fetch_action,              \
      HPX_PP_CAT(__dist_object_part_fetch_action_, type));

/**/

#define REGISTER_DIST_OBJECT_PART(type)                                       \
  HPX_REGISTER_ACTION(                                                        \
      dist_object::server::dist_object_part<type>::fetch_action,              \
      HPX_PP_CAT(__dist_object_part_fetch_action_, type));                    \
  typedef ::hpx::components::component<                                       \
      dist_object::server::dist_object_part<type>>                            \
      HPX_PP_CAT(__dist_object_part_, type);                                  \
  HPX_REGISTER_COMPONENT(HPX_PP_CAT(__dist_object_part_, type))               \
  /**/
#endif
