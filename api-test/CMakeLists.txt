add_executable(
    api-test
    main.cpp
    vk-device.cpp
    command-buffer.cpp
    vk-bindless.cpp
    vk-utils.cpp
    dx12-device.cpp
    ${CMAKE_SOURCE_DIR}/src/window.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/vulkan-loader.cpp
    ${CMAKE_SOURCE_DIR}/src/gltf.cpp
    ${CMAKE_SOURCE_DIR}/src/vec3.cpp
    ${CMAKE_SOURCE_DIR}/src/mesh.cpp
    ${CMAKE_SOURCE_DIR}/src/scene.cpp
    ${CMAKE_SOURCE_DIR}/src/camera.cpp
    ${CMAKE_SOURCE_DIR}/src/bvh.cpp
)

target_compile_definitions(api-test PUBLIC WINDOWS VK_NO_PROTOTYPES VK_USE_PLATFORM_WIN32_KHR API_TEST NOMINMAX)

target_compile_options(api-test
    PRIVATE -Wall -Wextra -msse4.1
)

target_include_directories(api-test SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})
target_include_directories(api-test SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty)
target_include_directories(api-test PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(api-test PUBLIC ${CMAKE_SOURCE_DIR}/include/api-test)
target_link_libraries(api-test PRIVATE imgui::imgui dxgi d3d12)
