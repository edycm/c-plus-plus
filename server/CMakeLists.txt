cmake_minimum_required(VERSION 2.8)

project(websocket_server)

set (TLS 1)

set (WEBSOCKETPP_BOOST_LIBS system thread random unit_test_framework)

#find_package (Boost 1.39.0 COMPONENTS "${WEBSOCKETPP_BOOST_LIBS}")
#include_directories (${Boost_INCLUDE_DIRS})
#link_directories (${Boost_LIBRARY_DIRS})

#set(OpenSSL_DIR "/home/cys/openssl")
#find_package(OpenSSL REQUIRED 1.0.1)

#include_directories("/home/cys/openssl-OpenSSL_1_1_0l/include")
#link_directories("/homde/cys/openssl-OpenSSL_1_1_0l")

#include_directories("/home/cys/websocket/websocketpp-master")

file (GLOB worker_file "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
list (REMOVE_ITEM worker_file "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp" )

message(STATUS "worker_file: ${worker_file}")


add_library(ws_server STATIC ${worker_file})
target_link_libraries(ws_server pthread boost_system ssl crypto dl)

include_directories(/home/cys/openssl/include)
link_directories("/home/cys/openssl")

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ws_server ssl crypto dl)
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")
