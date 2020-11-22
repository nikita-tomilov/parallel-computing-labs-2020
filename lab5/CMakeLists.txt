cmake_minimum_required(VERSION 3.15)
project(lab5)

set(CMAKE_C_STANDARD 11)
add_compile_options("-O3")
add_compile_options("-Wall")
add_compile_options("-Werror")
add_compile_options("-fopenmp")

add_executable(lab5
        lab5.c)

target_link_libraries(lab5 m)
target_link_libraries(lab5 pthread)