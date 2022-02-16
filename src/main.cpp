#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cassert>

#include "thirdparty/renderdoc.h"
#include "imgui.h"

#include "compute-renderpass.hpp"
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

    vkrenderer renderer { wnd };

    auto main_scene = scene(cam, width, height, renderer);

    std::string raytracing_shader_name = "compute";
    auto raytracing_pass = renderer.create_compute_renderpass();
    raytracing_pass->set_pipeline(raytracing_shader_name);

    auto* accumulation_texture = renderer.create_2d_texture(width, height, VK_FORMAT_R32G32B32A32_SFLOAT);
    auto* output_texture = renderer.create_2d_texture(width, height, VK_FORMAT_R32G32B32A32_SFLOAT);
    raytracing_pass->set_constant(8, main_scene.bvh_buffer);
    raytracing_pass->set_constant(16, main_scene.indices_buffer);
    raytracing_pass->set_constant(24, main_scene.positions_buffer);
    raytracing_pass->set_constant(32, main_scene.normals_buffer);
    raytracing_pass->set_constant(40, main_scene.uvs_buffer);
    raytracing_pass->set_constant(48, main_scene.materials_buffer);


    auto can_render = true;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = (float)width;
    io.DisplaySize.y = (float)height;

    while(wnd.isOpen) {
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

                    main_scene.meta.width = width;
                    main_scene.meta.height = height;

                    io.DisplaySize.x = (float)width;
                    io.DisplaySize.y = (float)height;

                    if (event.width == 0 && event.height == 0) {
                        can_render = false;
                    } else {
                        can_render = true;
                        renderer.recreate_swapchain();
                        main_scene.meta.cam.set_aspect_ratio((float)event.width / (float)event.height);
                        main_scene.meta.sample_index = 1;

                        accumulation_texture = renderer.create_2d_texture(width, height, VK_FORMAT_R32G32B32A32_SFLOAT);
                        output_texture = renderer.create_2d_texture(width, height, VK_FORMAT_R32G32B32A32_SFLOAT);
                        raytracing_pass->set_dispatch_size(event.width / 8 + 1, event.height / 8 + 1, 1);
                    }

                    main_scene.meta.sample_index = 1;

                    break;
                }
                case EVENT_TYPES::KEY_PRESS: {
                    auto move_speed = 1.f * delta_time;
                    if (wnd.keyboard[KEYS::LSHIFT] || wnd.keyboard[KEYS::RSHIFT]) {
                        move_speed *= 10.f;
                    }

                    vec3 move_vec {};

                    switch (event.key) {
                        case KEYS::W: {
                            move_vec = main_scene.meta.cam.forward * move_speed;
                            break;
                        }
                        case KEYS::S: {
                            move_vec = -main_scene.meta.cam.forward * move_speed;
                            break;
                        }
                        case KEYS::D: {
                            move_vec = main_scene.meta.cam.right * move_speed;
                            break;
                        }
                        case KEYS::A: {
                            move_vec = -main_scene.meta.cam.right * move_speed;
                            break;
                        }
                        case KEYS::E: {
                            main_scene.meta.cam.rotate_y(0.001f * delta_time);
                            main_scene.meta.sample_index = 1;
                            break;
                        }
                        case KEYS::Q: {
                            main_scene.meta.cam.rotate_y(-0.001f * delta_time);
                            main_scene.meta.sample_index = 1;
                            break;
                        }
                        case KEYS::SPACE: {
                            move_vec = main_scene.meta.cam.up * move_speed;
                            break;
                        }
                        case KEYS::LCTRL: {
                            move_vec = -main_scene.meta.cam.up * move_speed;
                            break;
                        }
                        default:
                            break;
                    }

                    main_scene.meta.cam.move(move_vec);

                    if (!move_vec.near_zero()) {
                        main_scene.meta.sample_index = 1;
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
        ImGui::Text("frame count %u\n", main_scene.meta.sample_index);

        if (ImGui::Checkbox("depth of field", (bool*)&main_scene.meta.enable_dof)) {
            main_scene.meta.sample_index = 1;
        }

        ImGui::SliderInt("max bounces", (int32_t*)&main_scene.meta.max_bounce, 1, 250);
        ImGui::SliderInt("min bounces", (int32_t*)&main_scene.meta.min_bounce, 1, main_scene.meta.max_bounce);
        if (ImGui::SliderInt("downscale factor", (int32_t*)&main_scene.meta.downscale_factor, 1, 32)) {
            main_scene.meta.sample_index = 1;
        }

        if (ImGui::Checkbox("debug bvh", (bool*)&main_scene.meta.debug_bvh)) {
            main_scene.meta.sample_index = 1;
        }

        ImGui::End();

        ImGui::EndFrame();

        // To start a frame capture, call StartFrameCapture.
        // You can specify NULL, NULL for the device to capture on if you have only one device and
        // either no windows at all or only one window, and it will capture from that device.
        // See the documentation below for a longer explanation
        // if(rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);

        //     renderer.ui();

        if (can_render) {
            std::memcpy(main_scene.scene_buffer->ptr(), &main_scene.meta, sizeof(main_scene.meta));

            raytracing_pass->set_ouput_texture(output_texture);
            raytracing_pass->set_constant(0, main_scene.scene_buffer);
            raytracing_pass->set_constant(60, accumulation_texture);


            renderer.render();

            main_scene.meta.sample_index++;

            std::swap(output_texture, accumulation_texture);
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
