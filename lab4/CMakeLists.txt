cmake_minimum_required(VERSION 3.15)
project(lab4)

find_package(OpenMP REQUIRED)

set(CMAKE_C_STANDARD 11)
add_compile_options("-O3")
add_compile_options("-Wall")
add_compile_options("-Werror")
add_compile_options("-fopenmp")

add_executable(lab4
        lab4.c)

target_link_libraries(lab4 m)
target_link_libraries(lab4 "${OpenMP_CXX_FLAGS}")