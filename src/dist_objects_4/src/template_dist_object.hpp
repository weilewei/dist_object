//  Copyright (c) 2019 Weile Wei
//  Copyright (c) 2019 Maxwell Resser
//  Copyright (c) 2019 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0328PM)
#define HPX_TEMPLATE_DIST_OBJECT_SERVER_MAR_20_2019_0328PM


#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/parallel_for_each.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/bind.hpp>

#include <hpx/lcos/barrier.hpp>

#include "server/template_dist_object.hpp"

#include <string>
#include <utility>
#include <chrono>
#include <thread>

namespace dist_object {
	enum class construction_type{ Meta_Object, All_to_All };
}

namespace dist_object {
	class meta_object_server : public hpx::components::locking_hook<
		hpx::components::component_base<meta_object_server>> {
	public:

		meta_object_server() : b(hpx::find_all_localities().size()){
			
		}

		std::vector<hpx::id_type> get_server_list() {
			if (servers.size() == hpx::find_all_localities().size()) {
				num_sent++;
				return servers;
			}
			std::vector<hpx::id_type> empty;
			empty.resize(0);
			return empty;
		}

		std::vector<hpx::id_type> registration(hpx::id_type id) {
			{
				std::lock_guard<hpx::lcos::local::spinlock> l(lk);
				servers.push_back(id);
			}
			b.wait();
			return servers;
		}

		int get_num() {
			return num_sent;
		}

		HPX_DEFINE_COMPONENT_ACTION(meta_object_server, get_server_list);
		HPX_DEFINE_COMPONENT_ACTION(meta_object_server, registration);
		HPX_DEFINE_COMPONENT_ACTION(meta_object_server, get_num);
			   
	private:
		int num_sent;
		hpx::lcos::local::barrier b;
		hpx::lcos::local::spinlock lk;
		std::vector<hpx::id_type> servers;
	};
	

}


typedef dist_object::meta_object_server::get_server_list_action get_list_action;
HPX_REGISTER_ACTION_DECLARATION(get_list_action, get_server_list_mo_action);
typedef dist_object::meta_object_server::registration_action register_with_meta_action;
HPX_REGISTER_ACTION_DECLARATION(register_with_meta_action, register_mo_action);
typedef dist_object::meta_object_server::get_num_action get_num_sent_action;
HPX_REGISTER_ACTION_DECLARATION(get_num_sent_action, get_num_action);


namespace dist_object {
	class meta_object : hpx::components::client_base<meta_object, meta_object_server> {
	public:
		typedef hpx::components::client_base<meta_object, meta_object_server> base_type;

		meta_object(std::string basename) : base_type(hpx::new_<meta_object_server>(hpx::find_here())){
			if (hpx::get_locality_id() == 0) {
				hpx::register_with_basename(basename, get_id(),hpx::get_locality_id());
			}
			meta_object_0 = hpx::find_from_basename(basename, 0).get();
		}

		hpx::future<std::vector<hpx::id_type>> get_server_list() {

			return hpx::async(get_list_action(), meta_object_0);
		}

		std::vector<hpx::id_type> registration(hpx::id_type id) {
			return hpx::async(register_with_meta_action(), meta_object_0, id).get();
		}

		hpx::future<int> get_num() {
			return hpx::async(get_num_sent_action(), meta_object_0);
		}

	private:
		hpx::id_type meta_object_0;
		//hpx::future<hpx::id_type> meta_object_;
		//hpx::id_type unwrapped_mo;
		//bool mo_is_gotten;
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
		static hpx::future<hpx::id_type> create_server(hpx::id_type where,
			Arg &&value) {
			return hpx::new_<server::partition<T>>(where, std::forward<Arg>(value));
		}

	public:
		dist_object() {}

		dist_object(std::string base, data_type const &data, construction_type type)
			: base_type(create_server(hpx::find_here(), data)) {

			size_t num_locs = hpx::find_all_localities().size();
			size_t here_ = hpx::get_locality_id();
			if (type == construction_type::Meta_Object) {
				meta_object mo(base);
				unwrapped = mo.registration(get_id());
				is_gotten.resize(num_locs);
				for (int i = 0; i < num_locs; i++) {
					is_gotten[i] = true;
				}
			}
			else if(type == construction_type::All_to_All) {
				hpx::register_with_basename(base + std::to_string(here_), get_id());
				std::vector<hpx::future<hpx::id_type>> locs_tmp;
				locs_tmp.resize(num_locs);
				unwrapped.resize(num_locs);
				is_gotten.resize(num_locs);
				for (int i = 0; i < num_locs; i++) {
					is_gotten[i] = false;
					locs_tmp[i] = hpx::find_from_basename(base + std::to_string(i), i);
				}
				// Should this be a parallel for loop? Insertion complicates it
				for (int i = 0; i < locs_tmp.size(); i++) {
					unwrapped[i] = locs_tmp[i].get();
					is_gotten[i] = true;
				}
			}
			else {
				// throw type not valid error;
			}
		}

		dist_object(std::string base, data_type const &data)
			: base_type(create_server(hpx::find_here(), data)) {
			size_t num_locs = hpx::find_all_localities().size();
			hpx::register_with_basename(base + std::to_string(hpx::get_locality_id()), get_id());
			locs.resize(num_locs);
			unwrapped.resize(num_locs);
			is_gotten.resize(num_locs);
			for (int i = 0; i < num_locs; i++) {
				is_gotten[i] = false;
				locs[i] = hpx::find_from_basename(base + std::to_string(i), i);
			}
		}

		dist_object(data_type &data)
			: base_type(create_server(hpx::find_here(), data)) {}

		dist_object(hpx::id_type where, data_type &&data)
			: base_type(create_server(where, std::move(data))) {}

		dist_object(hpx::future<hpx::id_type> &&id) : base_type(std::move(id)) {}

		dist_object(hpx::id_type &&id) : base_type(std::move(id)) {}

		void register_with_meta_object(std::string base) {
			int num_locs = hpx::find_all_localities().size();
			int here_ = hpx::get_locality_id();
			meta_object mo(base);
			hpx::id_type tmp = get_id();
			unwrapped = mo.registration(tmp);
			is_gotten.resize(num_locs);
			for (int i = 0; i < num_locs; i++) {
				is_gotten[i] = true;
			}
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

		hpx::future<data_type> fetch(int id)
		{
			HPX_ASSERT(this->get_id());
			hpx::id_type serv;
			std::cout << "Id is " << id << std::endl;
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
