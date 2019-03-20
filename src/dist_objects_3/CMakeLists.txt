cmake_minimum_required(VERSION 3.11)

PROJECT(dist_objects_3 CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)

ADD_SUBDIRECTORY( src )