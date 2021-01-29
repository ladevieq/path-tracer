cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

project(path-tracer VERSION 1.0 LANGUAGES CXX)

find_package(Vulkan REQUIRED)

if(UNIX)
    find_package(X11 REQUIRED)
endif()

add_subdirectory(src)
add_subdirectory(shaders)
