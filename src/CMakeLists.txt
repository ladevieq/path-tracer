include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Vulkan_INCLUDE_DIRS})

add_executable(
    path-tracer
    main.cpp
    vec3.cpp
    color.cpp
    ray.cpp
    sphere.cpp
    hittable_list.cpp
    camera.cpp
    lambertian.cpp
    metal.cpp
    dielectric.cpp
    vulkan-loader.cpp
    utils.cpp
    vk-renderer.cpp
)

if(UNIX)
    target_link_libraries(path-tracer ${CMAKE_DL_LIBS})
endif()
