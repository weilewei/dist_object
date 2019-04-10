# dist_object
`dist_object` implementation using HPX component

# Distributed Objects
- A distributed object is a single logical object partitioned across a set of localities/machines/nodes
- Any C++ type can be constructed into a distributed object
- Any existing C++ type object can be wraped into or referred to a distributed object
- `dist_object` can be made with two options: `all-to-all` (defualt) and `meta_object` 

# Significance
- Programming Productivity
- User Portability
- Driving force for RDMA

# Usage
## Construct `dist_object<T>`

## Construct `dist_object<T&>`


# Example - Matrix Transpose

