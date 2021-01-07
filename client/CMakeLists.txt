cmake_minimum_required(VERSION 2.8)

project("coap_client")


file(GLOB coap_srcs ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB coap_incs ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
list(REMOVE_ITEM coap_srcs ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)


add_executable(${PROJECT_NAME} ${coap_srcs} ${coap_incs})
target_link_libraries(${PROJECT_NAME} pthread coap-2)
