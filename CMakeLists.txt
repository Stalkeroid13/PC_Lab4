cmake_minimum_required(VERSION 4.1)
project(PC_Lab4)

set(CMAKE_CXX_STANDARD 20)

add_library(common INTERFACE)
target_include_directories(common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_executable(server server/main.cpp)
target_link_libraries(server PRIVATE common ws2_32)

add_executable(client client/main.cpp)
target_link_libraries(client PRIVATE common ws2_32)