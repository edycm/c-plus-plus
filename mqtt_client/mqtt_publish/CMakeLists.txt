cmake_minimum_required(VERSION 2.8)

project(mqtt_publish)

file (GLOB worker "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(${PROJECT_NAME} ${worker})
target_link_libraries(${PROJECT_NAME} mosquitto pthread)
