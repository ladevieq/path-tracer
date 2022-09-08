cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(CMAKE_BUILD_TYPE Debug)

project(path-tracer VERSION 1.0 LANGUAGES CXX)


find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()

# find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
# if (CMAKE_CXX_CPPCHECK)
#     file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/cppcheck)
# 
#     list(
#         APPEND CMAKE_CXX_CPPCHECK
#             "--project=${CMAKE_BINARY_DIR}/compile_commands.json"
#             "--cppcheck-build-dir=${CMAKE_BINARY_DIR}/cppcheck"
#             "--enable=all"
#             "-ithirdparty"
#     )
# endif()

add_subdirectory(src)
add_subdirectory(shaders)
