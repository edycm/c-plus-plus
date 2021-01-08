cmake_minimum_required(VERSION 2.8)

set (CMAKE_CXX_FLAGS "-g -o0 -Wall -std=c++11 -fPIC")

add_subdirectory(mqtt_subscribe)
add_subdirectory(mqtt_publish)
