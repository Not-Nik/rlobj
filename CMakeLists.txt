cmake_minimum_required(VERSION 3.19)
project(rlobj C)

set(CMAKE_C_STANDARD 99)

add_subdirectory(raylib)

add_library(rlobj rlobj.c rlobj.h)
target_link_libraries(rlobj raylib)

add_executable(test test.c)
target_link_libraries(test rlobj)
