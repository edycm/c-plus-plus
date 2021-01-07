cmake_minimum_required(VERSION 2.8)


set (CMAKE_CXX_FLAGS " -std=c++11 -g -O0 -fPIC")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/")
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/")

add_subdirectory(server)
add_subdirectory(client)
