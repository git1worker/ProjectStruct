cmake_minimum_required(VERSION 3.5)

project(cbasis)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(libs/pilink)
add_subdirectory(apps/mpl1c)
