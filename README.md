# dist_object
dist_object implementation for HPX

This is a dist_object example using HPX component.

A distributed object is a single logical object partitioned across 
a set of localities. (A locality is a single node in a cluster or a 
NUMA domian in a SMP machine.) Each locality constructs an instance of
dist_object<T>, where a value of type T represents the value of this
this locality's instance value. Once dist_object<T> is conctructed, it
has a universal name which can be used on any locality in the given
localities to locate the resident instance.
