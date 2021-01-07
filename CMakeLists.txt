cmake_minimum_required(VERSION 2.8)


set (CMAKE_CXX_FLAGS "-std=c++11 -g -o0 -fPIC -Wall")
set (CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wl,-rpath=${CMAKE_CURRENT_SOURCE_DIR}/lib")
set (CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wl,-rpath=./lib")

set (LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set (EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)

add_definitions(-DTLS)

add_subdirectory(server)
add_subdirectory(client)
