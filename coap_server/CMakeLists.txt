cmake_minimum_required(VERSION 2.8)


set (CMAKE_CXX_FLAGS " -std=c++11 -g -O0 -fPIC")

set (BUILD_HOME "/home/cys/coap")

include_directories("${BUILD_HOME}/libcoap/include/")
#include_directories("/home/cys/coap/docker_build/coap_server/json")
link_directories("${BUILD_HOME}/libcoap/build/lib/")


include_directories("${BUILD_HOME}/jsoncpp/include/")
link_directories("${BUILD_HOME}/jsoncpp/build/lib/")


add_subdirectory(server)
add_subdirectory(client)
