add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)

set(LIBRARIES imgui::imgui nlohmann_json::nlohmann_json Tracy::TracyClient)

add_executable(
    path-tracer
    main.cpp
    vec3.cpp
    color.cpp
    camera.cpp
    vulkan-loader.cpp
    utils.cpp
    vk-renderer.cpp
    vk-context.cpp
    vk-api.cpp
    window.cpp
    scene.cpp
    gltf.cpp
    compute-renderpass.cpp
    primitive-renderpass.cpp
    ${Tracy_SOURCE_DIR}/TracyClient.cpp
)

target_include_directories(path-tracer SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty)

target_include_directories(path-tracer PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(path-tracer PUBLIC ${Vulkan_INCLUDE_DIRS})

if(UNIX AND NOT APPLE)
    target_include_directories(path-tracer PUBLIC ${X11_xcb_INCLUDE_PATH})
endif()


if(UNIX AND NOT APPLE)
    list(APPEND LIBRARIES ${CMAKE_DL_LIBS} ${X11_xcb_LIB} ${X11_xkbcommon_LIB} ${X11_xkbcommon_X11_LIB})
endif()

target_link_libraries(path-tracer PRIVATE ${LIBRARIES})
