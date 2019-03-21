//  Copyright (c) 2019 Weile Wei
//  Copyright (c) 2019 Maxwell Resser
//  Copyright (c) 2019 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0328PM)
#define HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0328PM

#include <hpx/include/components.hpp>
#include <hpx/util/assert.hpp>

#include "server/template_dist_object.hpp"

#include <string>
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

		dist_object(std::string base, data_type const &data)
			: base_type(create_server(hpx::find_here(), data)) {
			int num_locs = hpx::find_all_localities().size();
			hpx::register_with_basename(base + std::to_string(hpx::get_locality_id()), get_id());
			locs.resize(num_locs);
			unwrapped.resize(num_locs);
			is_gotten.resize(num_locs);
			for (int i = 0; i < num_locs; i++) {
				is_gotten[i] = false;
				if (i == hpx::get_locality_id()) {
					continue;
				}
				locs[i] = hpx::find_from_basename(base + std::to_string(i), i);
			}
		}

		dist_object(hpx::id_type where, data_type &&data)
			: base_type(create_server(where, std::move(data))) {}

		dist_object(hpx::future<hpx::id_type> &&id) : base_type(std::move(id)) {}

		dist_object(hpx::id_type &&id) : base_type(std::move(id)) {}

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

		hpx::future<data_type> fetch(int id)
		{
			HPX_ASSERT(this->get_id());
			hpx::id_type serv;
			if (is_gotten[id])
				serv = unwrapped[id];
			else {
				serv = locs[id].get();
				unwrapped[id] = serv;
			}

			typedef typename server::partition<T>::fetch_action
				action_type;
			return hpx::async<action_type>(serv);
		}

	private:
		mutable std::shared_ptr<server::partition<T>> ptr;
		std::vector<hpx::future<hpx::id_type>> locs;
		std::vector<hpx::id_type> unwrapped;
		std::vector<bool> is_gotten;
		void ensure_ptr() const {
			if (!ptr) {
				ptr = hpx::get_ptr<server::partition<T>>(hpx::launch::sync, get_id());
			}
		}
	};
}

#endif
