ADD_EXECUTABLE(dist_objects_1 dist_objects_1.cpp)

# build your application using HPX
hpx_setup_target(dist_objects_1 
COMPONENT_DEPENDENCIES iostreams)