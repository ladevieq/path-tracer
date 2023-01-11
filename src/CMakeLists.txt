set(LIBRARIES imgui::imgui)

set(DEFINES -DNOMINMAX -D_USE_MATH_DEFINES -DVK_NO_PROTOTYPES)

if(UNIX AND NOT APPLE)
    set(DEFINES ${DEFINES} -DLINUX -DVK_USE_PLATFORM_XCB_KHR)
elseif(WIN32)
    set(DEFINES ${DEFINES} -DWINDOWS -DVK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
    set(DEFINES ${DEFINES} -DMACOS -DVK_USE_PLATFORM_METAL_EXT)
endif()


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
    bvh.cpp
    mesh.cpp
    resource-manager.cpp
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(
        path-tracer
        PRIVATE
        # -fsanitize=address TODO: enable once compiling with the flag works
        -fcolor-diagnostics -fansi-escape-codes # colored output when using ninja
        -ftime-trace
        -msse4.1
        # https://github.com/cpp-best-practices/cppbestpractices/blob/master/02-Use_the_Tools_Available.md#gcc--clang
        -Wall
        -Wextra
        -Wshadow
        -Wcast-align
        -Wnon-virtual-dtor
        -Wpedantic
        -Wunused
    )

# target_link_options(path-tracer PRIVATE -fsanitize=address)
endif()

target_link_directories(path-tracer PRIVATE "C:\\Program Files\\LLVM\\lib\\clang\\14.0.6\\lib\\windows")

target_compile_definitions(path-tracer PUBLIC ${DEFINES})

target_include_directories(path-tracer SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty)

target_include_directories(path-tracer SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})

target_include_directories(path-tracer PUBLIC ${CMAKE_SOURCE_DIR}/include)

if(UNIX AND NOT APPLE)
    target_include_directories(path-tracer PUBLIC ${X11_xcb_INCLUDE_PATH})
endif()

if(UNIX AND NOT APPLE)
    list(APPEND LIBRARIES ${CMAKE_DL_LIBS} ${X11_xcb_LIB} ${X11_xkbcommon_LIB} ${X11_xkbcommon_X11_LIB})
endif()

target_link_libraries(path-tracer PRIVATE ${LIBRARIES})
