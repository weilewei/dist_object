# dist_object
`dist_object` implementation using HPX component

# Distributed Objects
Distributed object in HPX: a single logical object partitioned over a set of localities/nodes/machines, where every locality shares the same global name for the distributed object (i.e. a universal name), but owns its local value. In other words, local data of the distributed object can be different, but they share access to one another's data globally.
```cpp
dist_object<double> dist_double("a_unique_string", 42.0);
```
Each locality in a given collection of the localities must call a constructor for `dist_object<T>`, with a unique user-provided name (as a std::string) and a value of type T representing the locality’s local value for the distributed object. The name is required in order to enable HPX to globally registers and retrieve the distributed object.

The `run_accumulation_reduce_to_locality0_parallel` code below demonstrates a simple accumulation computation using the distributed object in HPX.

In this example, the distributed object is an integer over all localities, the value of the object in each locality is set to the index value of the locality, such that (`dist_int` in locality 0 is 0, `dist_int` in locality 1 is 1, etc.) Although the constructor for a distributed object is called collectively, it is not clear when the constructor on each given locality will complete its own construction. If we make a fetch call to a distributed object in a remote locality which has not been constructed or made ready it may cause errors. Thus, it is necessary to insert a barrier to ensure that the object has been constructed on all localities. A barrier is used to guarantee rank 0 will not access any remote instance of the distributed object until all the ranks have guaranteed their construction. Importantly,  the `fetch` is done asychronously, and returns a future representing a copy of the data which the given locality's distributed object owns. The verification portion of the code only runs on locality 0.

```cpp
// generate boilerplate code required for HPX to function properly on the given type for dist_object<T>
REGISTER_DIST_OBJECT_PART(int);

void run_accumulation_reduce_to_locality0_parallel() {
  using dist_object::dist_object;
  std::atomic<int> expect_res = 0;
  int target_res = 0;
  int num_localities = hpx::find_all_localities().size();
  int cur_locality = hpx::get_locality_id();

  // declare a distributed object on every locality
  dist_object<int> dist_int("dist_int", cur_locality);

  // create a barrier and wait for the distributed object to be constructed in
  // all localities
  hpx::lcos::barrier wait_for_construction("wait_for_construction",
                                           num_localities, cur_locality);
  wait_for_construction.wait();

  if (cur_locality == 0) {
    using hpx::parallel::for_each;
    using hpx::parallel::execution::par;
    auto range = boost::irange(0, num_localities);
    // compute expect result in parallel
    // locality 0 fetchs all values
    for_each(par, std::begin(range), std::end(range),
             [&](std::uint64_t b) { expect_res += dist_int.fetch(b).get(); });
    hpx::wait_all();
    // compute target result
    // to verify the accumulation results
    for (int i = 0; i < num_localities; i++) {
      target_res += i;
    }
    assert(expect_res == target_res);
  }
}
```

- A distributed object is a single logical object partitioned across a set of localities/machines/nodes
- Any existing C++ type object can be wrapped into or referred to by a distributed object
- `dist_object` can be made with two options: `all-to-all` (defualt) and `meta_object` 

# Significance
- Programming Productivity
- User Portability
- Driving force for RDMA

The distributed object could be the driving force of finalizing the construction of RDMA (Remote Direct Memory Access)-based communication. If one locality wants to apply an action to the same object yet in anther locality in the given collection of localities, there needs to be some mutually agreed-upon identifier for referring to that object. Currently, the process aforementioned is supported using HPX component, which exposes one member function as a component action. The communication pattern of the HPX component relies on AGAS (Active Global Address Space), and is not RDMA style. HPX currently has basic RDMA infrastructure and a needs to continuingly develop. The application and interface of `dist_object` provides a clear guidance and direction on how HPX should build the RDMA underneath infrastructure. 

# Usage
## Construct `dist_object<T>`
```cpp
// generate boilerplate code required for HPX to function properly
// on the given type for dist_object<T>
REGISTER_DIST_OBJECT_PART(int);

void run_dist_object_int() {
	using dist_object::dist_object;
	// Construct a distrtibuted object of type int in all provided localities
	// User needs to provide the distributed object with a unique basename 
	// and data for construction. The unique basename string enables HPX 
	// register and retrive the distributed object.
	dist_object<int> dist_int("a_unique_name_string", hpx::get_locality_id() + 42);
	// If there exists more than 2 (and include) localities, we are able to 
	// asychronously fetch a future of a copy of the instance of this 
	// distributed object associated with the given locality
	if (hpx::get_locality_id() >= 2) {
		assert(dist_int.fetch(1).get() == 43);
	}
}
```

## Construct `dist_object<T&>`


# Example - Matrix Transpose

