//  Copyright (c) 2019 Weile Wei
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/include/components.hpp>
#include <hpx/include/actions.hpp>

#include <hpx/util/detail/pp/cat.hpp>

#include <hpx/hpx.hpp>
#include <hpx/runtime/components/component_factory.hpp>
#include <hpx/runtime/get_ptr.hpp>

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

			template_dist_object() 
				: value_(0) 
			{}

			template_dist_object(T const& value) 
				: value_(value)
			{}

			argument_type& operator*()
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
			HPX_DEFINE_COMPONENT_ACTION(template_dist_object, fetch);

		private:
			argument_type value_;
		};
	}
}

#define REGISTER_TEMPLATE_DIST_OBJECT_DECLARATION(type)                        \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
                                                                               \
  HPX_REGISTER_ACTION_DECLARATION(                                             \
      examples::server::template_dist_object<type>::fetch_action,              \
      HPX_PP_CAT(__template_dist_object_fetch_action_, type));                 \
  /**/

#define REGISTER_TEMPLATE_DIST_OBJECT(type)                                    \
  HPX_REGISTER_ACTION(                                                         \
      examples::server::template_dist_object<type>::fetch_action,              \
      HPX_PP_CAT(__template_dist_object_fetch_action_, type));                 \
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

		dist_object(argument_type const& value)
			: base_type(hpx::new_<server::template_dist_object<T>>(hpx::find_here(), value))
		{}

		dist_object(hpx::future<hpx::id_type> &&id)
			: base_type(std::move(id))
		{}

		dist_object(hpx::id_type &&id)
			: base_type(std::move(id))
		{}
		//hpx::future<std::shared_ptr<base_type> >

		T& operator*()
		{
			HPX_ASSERT(this->get_id());
			if (!ptr) {
				ptr = hpx::get_ptr<server::template_dist_object<T>>(hpx::launch::sync, get_id());
			}
			return **ptr;
		}

		hpx::future<argument_type> fetch()
		{
			HPX_ASSERT(this->get_id());

			typedef typename server::template_dist_object<T>::fetch_action
				action_type;
			return hpx::async<action_type>(this->get_id());
		}
		std::shared_ptr<server::template_dist_object<T>> ptr;

	};
}

REGISTER_TEMPLATE_DIST_OBJECT(double);
REGISTER_TEMPLATE_DIST_OBJECT(int);
using myVectorInt = std::vector<int>;
REGISTER_TEMPLATE_DIST_OBJECT(myVectorInt);
using myMatrixInt = std::vector<std::vector<int>>;
REGISTER_TEMPLATE_DIST_OBJECT(myMatrixInt);

void run_dist_object_int() {
	typedef typename examples::server::template_dist_object<int> dist_object_type;
	typedef typename dist_object_type::argument_type argument_type;

	std::vector<hpx::id_type> localities = hpx::find_all_localities();

	examples::dist_object<int> dist_object =
		hpx::new_<dist_object_type>(localities.back());
	std::cout << dist_object.fetch().get() << std::endl;
}

void run_dist_object_vec() {
	typedef typename examples::server::template_dist_object<myVectorInt> dist_object_type;
	typedef typename dist_object_type::argument_type argument_type;

	std::vector<hpx::id_type> localities = hpx::find_all_localities();

	myVectorInt vec{ 5, 5, 5 };

	examples::dist_object<myVectorInt> dist_vector(vec);
	std::cout << dist_vector.fetch().get()[1] << std::endl;
}

void run_dist_object_matrix() {
	typedef typename examples::server::template_dist_object<myVectorInt> dist_object_type;
	typedef typename dist_object_type::argument_type argument_type;

	std::vector<hpx::id_type> localities = hpx::find_all_localities();

	std::vector<std::vector<int>> m(3, std::vector<int>(3, 1));

	examples::dist_object<myMatrixInt> dist_matrix(m);
	/*std::cout << dist_matrix.fetch().get()[1][1] << std::endl;*/

	for (int i = 0; i < m.size(); i++) {
		for (int j = 0; j < m[0].size(); j++) {
			(*dist_matrix)[i][j] += 1;
			std::cout << (*dist_matrix)[i][j] << "\t";
		}
		std::cout << std::endl;
	}
}

int hpx_main() {
	run_dist_object_int();
	run_dist_object_vec();
	run_dist_object_matrix();
	return hpx::finalize();
}

int main(int argc, char* argv[]) {
	std::vector<std::string> const cfg = {
	"hpx.os_threads=2"
	};
	return hpx::init(argc, argv, cfg);
}