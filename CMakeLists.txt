cmake_minimum_required(VERSION 3.19)
project(rlobj C)

set(CMAKE_C_STANDARD 99)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_subdirectory(raylib)
endif ()

add_library(rlobj rlobj.c rlobj.h)
target_include_directories(rlobj PUBLIC .)
target_link_libraries(rlobj raylib)

add_executable(test test.c)
target_link_libraries(test rlobj)
