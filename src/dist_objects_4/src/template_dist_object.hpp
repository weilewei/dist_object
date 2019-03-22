//  Copyright (c) 2019 Weile Wei
//  Copyright (c) 2019 Maxwell Reesser
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
		static hpx::future<hpx::id_type> create_server(Arg &&value) {
			return hpx::new_<server::partition<T>>(hpx::find_here(), std::forward<Arg>(value));
		}

	public:
		dist_object() {}

		dist_object(std::string base, data_type const &data)
			: base_type(create_server(data)), base_(base) 
		{
			hpx::register_with_basename(base + std::to_string(hpx::get_locality_id()), get_id());
			basename_list.resize(hpx::find_all_localities().size());
		}

		dist_object(std::string base, data_type &&data)
			: base_type(create_server(std::move(data))), base_(base) 
		{
			hpx::register_with_basename(base + std::to_string(hpx::get_locality_id()), get_id());
		}

		dist_object(std::string base, hpx::future<hpx::id_type> &&id) 
			: base_type(std::move(id)), base_(base)
		{}

		dist_object(std::string base, hpx::id_type &&id) 
			: base_type(std::move(id)), base_(base)
		{}

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

		hpx::future<data_type> fetch(int idx)
		{
			HPX_ASSERT(this->get_id());
			hpx::id_type lookup = get_basename_helper(idx);
			typedef typename server::partition<T>::fetch_action
				action_type;
			return hpx::async<action_type>(lookup);
		}


	private:
		mutable std::shared_ptr<server::partition<T>> ptr;
		std::string base_;
		void ensure_ptr() const {
			if (!ptr) {
				ptr = hpx::get_ptr<server::partition<T>>(hpx::launch::sync, get_id());
			}
		}
	private:
		std::vector<hpx::id_type> basename_list;
		hpx::id_type get_basename_helper(int idx) {
			if (!basename_list[idx]) {
				basename_list[idx] = hpx::find_from_basename(base_ + std::to_string(idx), idx).get();
				return basename_list[idx];
			}
			return basename_list[idx];
		}
	};
}

#endif
