cmake_minimum_required(VERSION 3.15)
project(lab3_5)

find_package(OpenMP REQUIRED)

set(CMAKE_C_STANDARD 11)
add_compile_options("-O3")
add_compile_options("-Wall")
add_compile_options("-Werror")
add_compile_options("-fopenmp")

add_executable(lab3_5
        lab3.5.c)

target_link_libraries(lab3_5 "${OpenMP_CXX_FLAGS}")
