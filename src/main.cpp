#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cassert>

#include "thirdparty/renderdoc.h"
#include "imgui.h"

#include "vk-renderer.hpp"
#include "window.hpp"
#include "scene.hpp"
#include "defines.hpp"

#if defined(LINUX)
#include <unistd.h>
#include <dlfcn.h>
#elif defined(WINDOWS)
#include <Windows.h>
#endif

int main() {
    RENDERDOC_API_1_1_2 *rdoc_api = NULL;

#ifdef WINDOWS
    // At init, on windows
    if(HMODULE mod = GetModuleHandleA("renderdoc.dll"))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI =
            (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
        assert(ret == 1);
    }
#elif defined(LINUX)
    // At init, on linux/android.
    // For android replace librenderdoc.so with libVkLayer_GLES_RenderDoc.so
    if(void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
        assert(ret == 1);
    }
#endif

    // Image dimensions
    const float aspect_ratio = 16.0 / 9.0;
    uint32_t width = 400;
    uint32_t height = width / aspect_ratio;
    const uint32_t samples_per_pixel = 500;

    window wnd { width , height };

    float delta_time = 0.f;
    uint64_t frame_count = 0;
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();

    point3 position = { 13.0, 2.0, -3.0 };
    point3 target = { 0.0, 0.0, 0.0 };
    auto v_fov = 90;
    auto aperture = 0.1;
    auto focus_distance = 10;
    auto cam = camera(position, target, v_fov, (float)width / (float)height, aperture, focus_distance);
    auto main_scene = scene(cam, width, height);
    auto can_render = true;
    vkrenderer renderer { wnd, sizeof(main_scene.meta), sizeof(main_scene.triangles[1]) * main_scene.triangles.size(), sizeof(main_scene.packed_nodes[0]) * main_scene.packed_nodes.size() };
    // vkrenderer renderer { wnd, sizeof(main_scene.meta), sizeof(main_scene.spheres[0]) * main_scene.spheres.size(), sizeof(main_scene.nodes[0]) * main_scene.nodes.size() };

    // Upload scene content
    std::memcpy(renderer.scene_buffer_ptr(), &main_scene.meta, sizeof(main_scene.meta));
    renderer.update_geometry_buffer(main_scene.triangles.data());
    renderer.update_bvh_buffer(main_scene.packed_nodes.data());

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = (float)width;
    io.DisplaySize.y = (float)height;

    while(wnd.isOpen) {
        auto reset_accumulation = false;

        end = std::chrono::high_resolution_clock::now();

        delta_time = std::chrono::duration<float, std::milli>(end - start).count();

        start = std::chrono::high_resolution_clock::now();

        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = delta_time;

        wnd.poll_events();

        for (auto event: wnd.events) {
            switch(event.type) {
                case EVENT_TYPES::RESIZE: {
                    width = event.width;
                    height = event.height;

                    ((scene*) renderer.scene_buffer_ptr())->meta.width = width;
                    ((scene*) renderer.scene_buffer_ptr())->meta.height = height;

                    io.DisplaySize.x = (float)width;
                    io.DisplaySize.y = (float)height;

                    if (event.width == 0 && event.height == 0) {
                        can_render = false;
                    } else {
                        can_render = true;
                        renderer.recreate_swapchain();
                        ((scene*) renderer.scene_buffer_ptr())->meta.cam.set_aspect_ratio((float)event.width / (float)event.height);
                        ((scene*) renderer.scene_buffer_ptr())->meta.sample_index = 0;
                    }

                    reset_accumulation = true;

                    break;
                }
                case EVENT_TYPES::KEY_PRESS: {
                    auto move_speed = 0.01f * delta_time;
                    if (wnd.keyboard[KEYS::LSHIFT] || wnd.keyboard[KEYS::RSHIFT] || wnd.keyboard[KEYS::SHIFT]) {
                        move_speed *= 10.f;
                    }

                    vec3 move_vec {};

                    switch (event.key) {
                        case KEYS::W: {
                            move_vec = ((scene*) renderer.scene_buffer_ptr())->meta.cam.forward.unit() * move_speed;
                            break;
                        }
                        case KEYS::S: {
                            move_vec = -((scene*) renderer.scene_buffer_ptr())->meta.cam.forward.unit() * move_speed;
                            break;
                        }
                        case KEYS::D: {
                            move_vec = ((scene*) renderer.scene_buffer_ptr())->meta.cam.right.unit() * move_speed;
                            break;
                        }
                        case KEYS::A: {
                            move_vec = -((scene*) renderer.scene_buffer_ptr())->meta.cam.right.unit() * move_speed;
                            break;
                        }
                        case KEYS::E: {
                            ((scene*) renderer.scene_buffer_ptr())->meta.cam.rotate_y(0.001f * delta_time);
                            reset_accumulation = true;
                            break;
                        }
                        case KEYS::Q: {
                            ((scene*) renderer.scene_buffer_ptr())->meta.cam.rotate_y(-0.001f * delta_time);
                            reset_accumulation = true;
                            break;
                        }
                        case KEYS::SPACE: {
                            move_vec = ((scene*) renderer.scene_buffer_ptr())->meta.cam.up.unit() * move_speed;
                            break;
                        }
                        case KEYS::CTRL: {
                            move_vec = -((scene*) renderer.scene_buffer_ptr())->meta.cam.up.unit() * move_speed;
                            break;
                        }
                        default:
                            break;
                    }

                    ((scene*) renderer.scene_buffer_ptr())->meta.cam.move(move_vec);

                    if (!move_vec.near_zero()) {
                        reset_accumulation = true;
                    }

                    break;
                }
                case EVENT_TYPES::BUTTON_PRESS: {
                    if (event.button == BUTTONS::LEFT) {
                        io.MouseDown[0] = true;
                    }
                    if (event.button == BUTTONS::RIGHT) {
                        io.MouseDown[1] = true;
                    }
                    break;
                }
                case EVENT_TYPES::BUTTON_RELEASE: {
                    if (event.button == BUTTONS::LEFT) {
                        io.MouseDown[0] = false;
                    }
                    if (event.button == BUTTONS::RIGHT) {
                        io.MouseDown[1] = false;
                    }
                    break;
                }
                case EVENT_TYPES::MOUSE_MOVE: {
                    io.MousePos = ImVec2 {
                        (float)event.x,
                        (float)event.y
                    };
                    break;
                }
                default:
                    break;
            }
        }

        wnd.events.clear();

        // GUI
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({ 0.f, 0.f });
        ImGui::SetNextWindowSize({ 0.f, 0.f });
        ImGui::Begin("Debug", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        ImGui::Text("frame per second %u\n", static_cast<uint32_t>(1000.f / delta_time));
        ImGui::Text("frame time %f ms\n", delta_time);
        ImGui::Text("frame count %u\n", ((scene*) renderer.scene_buffer_ptr())->meta.sample_index);

        if (ImGui::Checkbox("depth of field", (bool*)&((scene*) renderer.scene_buffer_ptr())->meta.enable_dof)) {
            reset_accumulation = true;
        }

        ImGui::SliderInt("bounces", (int32_t*)&((scene*) renderer.scene_buffer_ptr())->meta.max_bounce, 1, 250);

        if (ImGui::Checkbox("debug bvh", (bool*)&((scene*) renderer.scene_buffer_ptr())->meta.debug_bvh)) {
            reset_accumulation = true;
        }

        ImGui::End();

        ImGui::EndFrame();

        // To start a frame capture, call StartFrameCapture.
        // You can specify NULL, NULL for the device to capture on if you have only one device and
        // either no windows at all or only one window, and it will capture from that device.
        // See the documentation below for a longer explanation
        // if(rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);

        if (can_render) {
            renderer.begin_frame();

            if (reset_accumulation) {
                ((scene*) renderer.scene_buffer_ptr())->meta.sample_index = 0;
                renderer.reset_accumulation();
            }

            renderer.compute(width, height);

            ((scene*) renderer.scene_buffer_ptr())->meta.sample_index++;

            renderer.ui();

            renderer.finish_frame();
        }

        // stop the capture
        // if(rdoc_api) rdoc_api->EndFrameCapture(NULL, NULL);

        frame_count++;
    }

    //-------------------------
    // Image exporter
    //-------------------------
    // PPM format header
    // std::cout << "P3\n" << width << '\n' << height << "\n255" << std::endl;
    // auto output_image = renderer.output_image();
    // for (size_t index = 0; index < height * width; index++) {
    //     write_color(std::cout, output_image[index]);
    // }


    std::cerr << "Done !" << std::endl;

    return 0;
}
