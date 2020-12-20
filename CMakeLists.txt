cmake_minimum_required(VERSION 3.10)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(path-tracer VERSION 1.0 LANGUAGES CXX)

add_subdirectory(src)
