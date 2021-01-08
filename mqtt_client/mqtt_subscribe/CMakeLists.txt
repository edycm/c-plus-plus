cmake_minimum_required(VERSION 2.8)

project(mqtt_consumer)

#set (CMAKE_CXX_FLAGS "-g -o0 -Wall -std=c++11 -fPIC")

file (GLOB worker "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

#include_directories(/home/cys/mqtt/mqtt_server/mosquitto/bin/include)
#link_directories(/home/cys/mqtt/mqtt_server/mosquitto/bin/lib)


add_executable(${PROJECT_NAME} ${worker})
target_link_libraries(${PROJECT_NAME} mosquitto)
