add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)

# VCPKG includes
include_directories(./vcpkg/installed/$ENV{VCPKG_TARGET_TRIPLET}/include)

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
    vk-context.cpp
    vk-api.cpp
    window.cpp
)

# VCPKG libs
link_directories(./vcpkg/installed/$ENV{VCPKG_TARGET_TRIPLET}/lib)

if(UNIX)
    target_link_libraries(path-tracer ${CMAKE_DL_LIBS} ${X11_xcb_LIB})
    target_link_libraries(path-tracer ${CMAKE_DL_LIBS} ${X11_xkbcommon_LIB})
    target_link_libraries(path-tracer ${CMAKE_DL_LIBS} ${X11_xkbcommon_X11_LIB})
endif()
