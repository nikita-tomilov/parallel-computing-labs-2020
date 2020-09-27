cmake_minimum_required(VERSION 3.15)
project(lab2 C)

set(CMAKE_C_STANDARD 11)
add_compile_options("-O3")
add_compile_options("-Wall")
add_compile_options("-Werror")
link_directories(${CMAKE_SOURCE_DIR}/.)

add_executable(lab2
        lab2.c)
target_link_libraries(lab2 m)
target_link_libraries(lab2 fwSignal)
target_link_libraries(lab2 fwBase)