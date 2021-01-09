include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Vulkan_INCLUDE_DIRS})

add_executable(
    path-tracer
    main.cpp
    vec3.cpp
    color.cpp
    sphere.cpp
    camera.cpp
    vulkan-loader.cpp
    utils.cpp
    vk-renderer.cpp
)

if(UNIX)
    target_link_libraries(path-tracer ${CMAKE_DL_LIBS})
endif()
