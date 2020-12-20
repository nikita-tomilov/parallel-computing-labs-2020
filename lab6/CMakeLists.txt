cmake_minimum_required(VERSION 3.15)
project(lab6)

set(CMAKE_C_STANDARD 11)
add_compile_options("-O3")
add_compile_options("-Wall")
add_compile_options("-Werror")
add_compile_options("-fopenmp")

add_executable(lab6
        lab6.c)

target_link_libraries(lab6 m)
target_link_libraries(lab6 pthread)
target_link_libraries(lab6 OpenCL)
