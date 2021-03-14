add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Vulkan_INCLUDE_DIRS})

if(${CMAKE_SYSTEM_NAME} EQUAL "Linux")
    include_directories(${X11_xcb_INCLUDE_PATH})
endif()

set(LIBRARIES imgui::imgui)

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

if(${CMAKE_SYSTEM_NAME} EQUAL "Linux")
    list(APPEND LIBRARIES ${CMAKE_DL_LIBS} ${X11_xcb_LIB} ${X11_xkbcommon_LIB} ${X11_xkbcommon_X11_LIB})
endif()

target_link_libraries(path-tracer PRIVATE ${LIBRARIES})
