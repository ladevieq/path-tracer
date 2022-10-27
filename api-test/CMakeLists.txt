add_executable(
    api-test
    main.cpp
    vk-device.cpp
    ${CMAKE_SOURCE_DIR}/src/window.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/vulkan-loader.cpp
)

target_compile_definitions(api-test PUBLIC WINDOWS VK_NO_PROTOTYPES VK_USE_PLATFORM_WIN32_KHR)

target_include_directories(api-test SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})
target_include_directories(api-test SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty)
target_include_directories(api-test PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(api-test PUBLIC ${CMAKE_SOURCE_DIR}/include/api-test)
