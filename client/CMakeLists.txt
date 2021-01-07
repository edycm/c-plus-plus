cmake_minimum_required(VERSION 2.8)

project(websocket_client)

#set(OpenSSL_DIR "/home/cys/openssl")
#find_package(OpenSSL REQUIRED 1.0.1)

include_directories("/home/cys/websocket/websocketpp-master")

file (GLOB worker_file "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
#list (REMOVE_ITEM worker_file "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
message (STATUS "worker_file: ${worker_file}")

add_library(ws_client ${worker_file})
target_link_libraries(ws_client dl pthread boost_system ssl crypto)

add_executable(${PROJECT_NAME} ${worker_file})
target_link_libraries(${PROJECT_NAME} ws_client)
#target_link_libraries(${PROJECT_NAME} boost_system dl ssl crypto pthread)
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
