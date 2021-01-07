cmake_minimum_required(VERSION 2.8)

project(websocket_client)

file (GLOB worker_file "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
list (REMOVE_ITEM worker_file "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(ws_client ${worker_file})
target_link_libraries(ws_client dl pthread boost_system ssl crypto)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ws_client)
