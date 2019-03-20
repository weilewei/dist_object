ADD_EXECUTABLE(dist_objects_3 dist_objects_3.cpp)

# build your application using HPX
hpx_setup_target(dist_objects_3 
COMPONENT_DEPENDENCIES iostreams)