//  Copyright (c) 2019 Weile Wei
//  Copyright (c) Maxwell Resser
//  Copyright (c) 2019 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0328PM)
#define HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0328PM

#include <hpx/include/components.hpp>
#include <hpx/util/assert.hpp>

#include "server/template_dist_object.hpp"

#include <utility>

namespace dist_object {
	template <typename T>
	class dist_object
		: hpx::components::client_base<dist_object<T>, server::partition<T>> {
		typedef hpx::components::client_base<dist_object<T>, server::partition<T>>
			base_type;

		typedef typename server::partition<T>::data_type data_type;

	private:
		template <typename Arg>
		static hpx::future<hpx::id_type> create_server(hpx::id_type where,
			Arg &&value) {
			return hpx::new_<server::partition<T>>(where, std::forward<Arg>(value));
		}

	public:
		dist_object() {}

		dist_object(hpx::id_type where, data_type const &data)
			: base_type(create_server(where, data)) {}

		dist_object(hpx::id_type where, data_type &&data)
			: base_type(create_server(where, std::move(data))) {}

		dist_object(hpx::future<hpx::id_type> &&id) : base_type(std::move(id)) {}

		dist_object(hpx::id_type &&id) : base_type(std::move(id)) {}

		~dist_object();
		size_t size() {
			HPX_ASSERT(this->get_id());
			ensure_ptr();
			return (**ptr).size();
		}

		data_type const &operator*() const {
			HPX_ASSERT(this->get_id());
			ensure_ptr();
			return **ptr;
		}

		data_type &operator*() {
			HPX_ASSERT(this->get_id());
			ensure_ptr();
			return **ptr;
		}

		data_type const* operator->() const
		{
			HPX_ASSERT(this->get_id());
			ensure_ptr();
			return &**ptr;
		}

		data_type* operator->()
		{
			HPX_ASSERT(this->get_id());
			ensure_ptr();
			return &**ptr;
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

#endif
