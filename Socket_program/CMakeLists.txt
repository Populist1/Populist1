cmake_minimum_required(VERSION 3.0)
project(Socket_program)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(Socket_program1 program1.cpp)
target_link_libraries(Socket_program1 -lpthread)
add_executable(Socket_program2 program2.cpp)
target_link_libraries(Socket_program2 -lpthread)