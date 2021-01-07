cmake_minimum_required(VERSION 2.8)


set (CMAKE_CXX_FLAGS "-std=c++11 -g -o0 -fPIC -Wall")
set (CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wl,-rpath=../lib")
set (CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wl,-rpath=./lib")
set (CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wl,-rpath=../../lib")

set (LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set (EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)
#include_directories("/home/cys/openssl/include")
#link_directories("/homde/cys/openssl")
#include_directories("/home/cys/openssl-OpenSSL_1_1_0l/include")
#link_directories("/homde/cys/openssl-OpenSSL_1_1_0l")

add_definitions(-DTLS)

add_subdirectory(server)
add_subdirectory(client)
