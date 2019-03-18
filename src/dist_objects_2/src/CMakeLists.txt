ADD_EXECUTABLE(dist_objects_2 dist_objects_2.cpp)

# build your application using HPX
hpx_setup_target(dist_objects_2 
COMPONENT_DEPENDENCIES iostreams)