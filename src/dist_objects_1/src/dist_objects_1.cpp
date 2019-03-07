//  Copyright (c) 2019 Weile Wei
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/include/components.hpp>
#include <hpx/include/actions.hpp>

#include <hpx/util/detail/pp/cat.hpp>

#include <hpx/hpx.hpp>
#include <hpx/runtime/components/component_factory.hpp>

namespace examples {
	namespace server
	{
		template <typename T>
		class template_dist_object
			: public hpx::components::locking_hook<
			hpx::components::component_base<template_dist_object<T> > >
		{
		public:
			typedef T argument_type;

			template_dist_object() : value_(5) {}

			argument_type query() const
			{
				return value_;
			}

			argument_type fetch() const
			{
				return value_;		
			}
			///////////////////////////////////////////////////////////////////////
			// Each of the exposed functions needs to be encapsulated into an
            // action type, generating all required boilerplate code for threads,
            // serialization, etc.
			HPX_DEFINE_COMPONENT_ACTION(template_dist_object, query);
			HPX_DEFINE_COMPONENT_ACTION(template_dist_object, fetch);

		private:
			argument_type value_;
		};
	}
}

#define REGISTER_TEMPLATE_DIST_OBJECT_DECLARATION(type)                        \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
      examples::server::template_dist_object<type>::query_action,              \
      HPX_PP_CAT(__template_dist_object_query_action_, type));                 \
                                                                               \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
      examples::server::template_dist_object<type>::fetch_action,              \
      HPX_PP_CAT(__template_dist_object_fetch_action_, type));                 \
  /**/

#define REGISTER_TEMPLATE_DIST_OBJECT(type)                                    \
  HPX_REGISTER_ACTION(                                                         \
      examples::server::template_dist_object<type>::query_action,              \
      HPX_PP_CAT(__template_dist_object_query_action_, type));                 \
  HPX_REGISTER_ACTION(                                                         \
      examples::server::template_dist_object<type>::fetch_action,              \
      HPX_PP_CAT(__template_dist_object_fetch_action_, type));                  \
  typedef ::hpx::components::component<                                        \
      examples::server::template_dist_object<type>>                            \
      HPX_PP_CAT(__template_dist_object_, type);                               \
  HPX_REGISTER_COMPONENT(HPX_PP_CAT(__template_dist_object_, type))            \
  /**/

HPX_REGISTER_COMPONENT_MODULE(); // not for exe

namespace examples {
	template <typename T>
	class dist_object
		: public hpx::components::client_base <
		dist_object<T>, server::template_dist_object<T>
		> 
	{
		typedef hpx::components::client_base<
			dist_object<T>, server::template_dist_object<T>
		> base_type;

		typedef typename server::template_dist_object<T>::argument_type
			argument_type;
	public:
		dist_object()
		{}

		dist_object(hpx::future<hpx::id_type> &&id)
			: base_type(std::move(id))
		{}

		dist_object(hpx::id_type &&id)
			: base_type(std::move(id))
		{}

		argument_type query(hpx::launch::sync_policy = hpx::launch::sync)
		{
			HPX_ASSERT(this->get_id());

			typedef typename server::template_dist_object<T>::query_action
				action_type;
			return action_type()(this->get_id());
		}

		hpx::future<argument_type> fetch()
		{
			HPX_ASSERT(this->get_id());

			typedef typename server::template_dist_object<T>::fetch_action
				action_type;
			return hpx::async<action_type>(this->get_id());
		}

		//argument_type fetch(hpx::id_type const& id, hpx::launch::sync_policy = hpx::launch::sync)
		//{
		//	HPX_ASSERT(this->get_id());

		//	typedef typename server::template_dist_object<T>::fetch_action
		//		action_type;
		//	return action_type(id)(this->get_id());
		//}
	};
}

REGISTER_TEMPLATE_DIST_OBJECT(double);
REGISTER_TEMPLATE_DIST_OBJECT(int);
//using myVectorInt = std::vector<int>;
//REGISTER_TEMPLATE_DIST_OBJECT(myVectorInt);

template <typename T>
void run_template_dist_object() {
	typedef typename examples::server::template_dist_object<T> dist_object_type;
	typedef typename dist_object_type::argument_type argument_type;

	std::vector<hpx::id_type> localities = hpx::find_all_localities();

	examples::dist_object<T> dist_object =
		hpx::new_<dist_object_type>(localities.back());
	std::cout << dist_object.fetch().get() << std::endl;
}

int hpx_main() {
	run_template_dist_object<int>();
	return hpx::finalize();
}

int main(int argc, char* argv[]) {
	std::vector<std::string> const cfg = {
	"hpx.os_threads=2"
	};
	return hpx::init(argc, argv, cfg);
}