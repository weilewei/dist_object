//  Copyright (c) 2019 Weile Wei
//  Copyright (c) 2019 Maxwell Reeser
//  Copyright (c) 2019 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0328PM)
#define HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0328PM

#include "server/template_dist_object.hpp"

#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/parallel_for_each.hpp>
#include <hpx/lcos/barrier.hpp>
#include <hpx/runtime/serialization/unordered_map.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/bind.hpp>

#include <chrono>
#include <numeric>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

namespace dist_object {
	enum class construction_type{ Meta_Object, All_to_All };
}

namespace dist_object {
	class meta_object_server : public hpx::components::locking_hook<
		hpx::components::component_base<meta_object_server>> {
	public:

		meta_object_server(std::size_t num_locs, std::size_t root) : b(num_locs), root_(root)
		{ 
			servers_ = std::unordered_map<std::size_t, hpx::id_type>();
			retrieved = false;
		}

		std::unordered_map<std::size_t, hpx::id_type> get_server_list() {
			return servers_;
		}

		//std::vector<hpx::id_type> get_server_list() {
		//	return servers__;
		//}

		std::unordered_map<std::size_t, hpx::id_type> registration(std::size_t source_loc, hpx::id_type id) 
		{
			{
				std::lock_guard<hpx::lcos::local::spinlock> l(lk);
				servers_[source_loc] = id;
			}
			b.wait();
			return servers_;
		}

		//std::vector< hpx::id_type> registration(std::size_t source_loc, hpx::id_type id)
		//{
		//	{
		//		std::lock_guard<hpx::lcos::local::spinlock> l(lk);
		//		servers__[source_loc] = id;
		//	}
		//	b.wait();
		//	return servers__;
		//}

		HPX_DEFINE_COMPONENT_ACTION(meta_object_server, get_server_list);
		HPX_DEFINE_COMPONENT_ACTION(meta_object_server, registration);
			   
	private:
		hpx::lcos::local::barrier b;
		hpx::lcos::local::spinlock lk;
		bool retrieved;
		std::size_t root_;
		std::unordered_map<std::size_t, hpx::id_type> servers_;
		std::vector<hpx::id_type> servers__;
	};
	

}


typedef dist_object::meta_object_server::get_server_list_action get_list_action;
HPX_REGISTER_ACTION_DECLARATION(get_list_action, get_server_list_mo_action);
typedef dist_object::meta_object_server::registration_action register_with_meta_action;
HPX_REGISTER_ACTION_DECLARATION(register_with_meta_action, register_mo_action);


namespace dist_object {
	class meta_object : hpx::components::client_base<meta_object, meta_object_server> {
	public:
		typedef hpx::components::client_base<meta_object, meta_object_server> base_type;
		
		meta_object(std::string basename, std::size_t num_locs, std::size_t root) : 
			base_type(hpx::new_<meta_object_server>(hpx::find_here(), num_locs, root))
		{
			if (hpx::get_locality_id() == root) {
				hpx::register_with_basename(basename, get_id(),hpx::get_locality_id());
			}
			meta_object_0 = hpx::find_from_basename(basename, root).get();
		}

		std::unordered_map<std::size_t, hpx::id_type> get_server_list() {

			return hpx::async(get_list_action(), meta_object_0).get();
		}

		//hpx::future<std::vector<hpx::id_type>> get_server_list() {

		//	return hpx::async(get_list_action(), meta_object_0);
		//}

		std::unordered_map<std::size_t, hpx::id_type> registration(hpx::id_type id) 
		{
			return hpx::async(register_with_meta_action(), meta_object_0, 
				hpx::get_locality_id(), id).get();
		}

		//std::vector<hpx::id_type> registration(hpx::id_type id)
		//{
		//	return hpx::async(register_with_meta_action(), meta_object_0,
		//		hpx::get_locality_id(), id).get();
		//}
		
	private:
		hpx::id_type meta_object_0;
	};
}

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
			return hpx::new_<server::partition<T>>(hpx::find_here(), 
				                                   std::forward<Arg>(value));
		}

	public:
		dist_object() {}

		dist_object(std::string base, data_type const &data, 
			        construction_type type, std::vector<size_t> localities)
			: base_type(create_server(data)), base_(base) 
		{
			assert(localities.size() > 0);
			bool here_listed = std::find(localities.begin(), localities.end(),
				hpx::get_locality_id()) != localities.end();

			std::sort(localities.begin(), localities.end());
			
			if (type == construction_type::Meta_Object) {
				meta_object mo(base, localities.size(), localities[0]);
				if(here_listed)
					locs =	mo.registration(get_id());
				else {
					locs = mo.get_server_list();
					while (locs.size() != localities.size()) {
						locs = mo.get_server_list();
					}
				}					
				basename_registration_helper(base);
			}
			else if (type == construction_type::All_to_All) {
				basename_registration_helper(base);
			}
			else {
				// throw type not valid error;
			}
		}

		dist_object(std::string base, data_type const &data, 
			construction_type type)
			: base_type(create_server(data)), base_(base) 
		{

			size_t num_locs = hpx::find_all_localities().size();
			std::vector<size_t> my_locs(num_locs);
			std::iota(my_locs.begin(), my_locs.end(), 0);
			size_t here_ = hpx::get_locality_id();
			if (type == construction_type::Meta_Object) {
				meta_object mo(base, num_locs, 0);
				locs = mo.registration(get_id());
				basename_registration_helper(base);
			}
			else if(type == construction_type::All_to_All) {
				basename_registration_helper(base);
			}
			else {
				// throw type not valid error;
			}
		}

		dist_object(std::string base, data_type const &data)
			: base_type(create_server(data)), base_(base)
		{
			basename_registration_helper(base);
		}

		dist_object(std::string base, data_type &&data)
			: base_type(create_server(std::move(data))), base_(base)
		{
			basename_registration_helper(base);
		}

		dist_object(std::string base, hpx::future<hpx::id_type> &&id)
			: base_type(std::move(id)), base_(base)
		{
			basename_registration_helper(base);
		}

		dist_object(std::string base, hpx::id_type &&id)
			: base_type(std::move(id)), base_(base)
		{

			basename_registration_helper(base);
		}

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
		std::unordered_map<std::size_t, hpx::id_type> locs;

		hpx::id_type get_basename_helper(int idx) {
			if (!locs[idx]) {
				//basename_list[idx] = hpx::find_from_basename(base_ + std::to_string(idx), idx).get();
				locs[idx] = hpx::find_from_basename(base_ + std::to_string(idx), idx).get();
			}
			//return basename_list[idx];
			return locs[idx];
		}
		void basename_registration_helper(std::string base) {
			hpx::register_with_basename(base + std::to_string(hpx::get_locality_id()), get_id());
		}
	};
}

#endif
