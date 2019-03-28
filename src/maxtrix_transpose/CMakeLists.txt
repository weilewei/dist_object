cmake_minimum_required(VERSION 3.11)

PROJECT(maxtrix_transpose CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)

ADD_SUBDIRECTORY( src )