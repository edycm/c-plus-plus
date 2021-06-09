cmake_minimum_required(VERSION 2.8)

set (CMAKE_CXX_FLAGS " -std=c++11 -fPIC")

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    add_definitions(-DDEBUG)
    message(STATUS "CMAKE_BUILD_TYPE=Debug -O0")
else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


#file(GLOB_RECURSE worker_srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB worker_srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
#file(GLOB_RECURSE worker_incs ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB worker_incs ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
list (REMOVE_ITEM worker_srcs "${CMAKE_CURRENT_SOURCE_DIR}/test_base64.cpp")



add_executable(test_base64 test_base64.cpp ${worker_srcs} ${worker_incs})
#target_link_libraries(test_base64 )
