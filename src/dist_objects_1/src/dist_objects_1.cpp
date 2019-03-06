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

			template_dist_object() : value_(0) {}

			argument_type query() const
			{
				return value_;
			}

			HPX_DEFINE_COMPONENT_ACTION(template_dist_object, query);

		private:
			argument_type value_;
		};
	}
}

#define REGISTER_TEMPLATE_DIST_OBJECT_DECLARATION(type)                       \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        examples::server::template_dist_object<type>::query_action,           \
        HPX_PP_CAT(__template_dist_object_query_action_, type));              \
/**/

#define REGISTER_TEMPLATE_DIST_OBJECT(type)                                   \
    HPX_REGISTER_ACTION(                                                      \
        examples::server::template_dist_object<type>::query_action,           \
        HPX_PP_CAT(__template_dist_object_query_action_, type));              \
    typedef ::hpx::components::component<                                     \
        examples::server::template_dist_object<type>                          \
    > HPX_PP_CAT(__template_dist_object_, type);                              \
    HPX_REGISTER_COMPONENT(HPX_PP_CAT(__template_dist_object_, type))         \
/**/

HPX_REGISTER_COMPONENT_MODULE();

namespace examples {
	template <typename T>
	class template_dist_object
		: public hpx::components::client_base <
		template_dist_object<T>, server::template_dist_object<T>
		> 
	{
		typedef hpx::components::client_base<
			template_dist_object<T>, server::template_dist_object<T>
		> base_type;

		typedef typename server::template_dist_object<T>::argument_type
			argument_type;
	public:
		template_dist_object()
		{}

		template_dist_object(hpx::future<hpx::id_type> &&id)
			: base_type(std::move(id))
		{}

		template_dist_object(hpx::id_type &&id)
			: base_type(std::move(id))
		{}

		argument_type query(hpx::launch::sync_policy = hpx::launch::sync)
		{
			HPX_ASSERT(this->get_id());

			typedef typename server::template_dist_object<T>::query_action
				action_type;
			return action_type()(this->get_id());
		}
	};
}

REGISTER_TEMPLATE_DIST_OBJECT(double);
REGISTER_TEMPLATE_DIST_OBJECT(int);

template <typename T>
void run_template_dist_object() {
	typedef typename examples::server::template_dist_object<T> dist_object_type;
	typedef typename dist_object_type::argument_type argument_type;

	std::vector<hpx::id_type> localities = hpx::find_all_localities();

	examples::template_dist_object<T> dist_object =
		hpx::new_<dist_object_type>(localities.back());

	std::cout << dist_object.query() << std::endl;
}

int hpx_main() {
	run_template_dist_object<int>();
	//run_template_dist_object<int>();
	return hpx::finalize();
}

int main(int argc, char* argv[]) {
	std::vector<std::string> const cfg = {
	"hpx.os_threads=2"
	};
	return hpx::init(argc, argv, cfg);
}