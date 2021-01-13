add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Vulkan_INCLUDE_DIRS})

if(UNIX)
    include_directories(${X11_xcb_INCLUDE_PATH})
endif()

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
    window.cpp
)

if(UNIX)
    target_link_libraries(path-tracer ${CMAKE_DL_LIBS} ${X11_xcb_LIB})
endif()
