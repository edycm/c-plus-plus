cmake_minimum_required(VERSION 2.8)

set (CMAKE_CXX_FLAGS "-g -o0 -Wall -std=c++11 -fPIC")

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
link_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_subdirectory(mqtt_subscribe)
add_subdirectory(mqtt_publish)
