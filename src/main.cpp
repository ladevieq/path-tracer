#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cassert>

#include "thirdparty/renderdoc.h"
#include "imgui.h"

#include "vk-renderer.hpp"
#include "vulkan-loader.hpp"
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

    auto raytracing_pass = renderer.create_compute_renderpass();
    raytracing_pass->set_pipeline(renderer.api.create_compute_pipeline("compute"));

    auto* accumulation_texture = renderer.create_2d_texture(width, height);
    auto* output_texture = renderer.create_2d_texture(width, height);
    raytracing_pass->set_constant(0, &main_scene.scene_buffer.device_address);
    raytracing_pass->set_constant(8, &main_scene.bvh_buffer.device_address);
    raytracing_pass->set_constant(16, &main_scene.indices_buffer.device_address);
    raytracing_pass->set_constant(24, &main_scene.positions_buffer.device_address);
    raytracing_pass->set_constant(32, &main_scene.normals_buffer.device_address);
    raytracing_pass->set_constant(40, &main_scene.uvs_buffer.device_address);
    raytracing_pass->set_constant(48, &main_scene.materials_buffer.device_address);


    auto can_render = true;

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

                    ((scene*) main_scene.scene_buffer_ptr())->meta.width = width;
                    ((scene*) main_scene.scene_buffer_ptr())->meta.height = height;

                    io.DisplaySize.x = (float)width;
                    io.DisplaySize.y = (float)height;

                    if (event.width == 0 && event.height == 0) {
                        can_render = false;
                    } else {
                        can_render = true;
                        renderer.recreate_swapchain();
                        ((scene*) main_scene.scene_buffer_ptr())->meta.cam.set_aspect_ratio((float)event.width / (float)event.height);
                        ((scene*) main_scene.scene_buffer_ptr())->meta.sample_index = 0;

                        accumulation_texture = renderer.create_2d_texture(width, height);
                        output_texture = renderer.create_2d_texture(width, height);
                        raytracing_pass->set_dispatch_size(event.width / 8 + 1, event.height / 8 + 1, 1);
                    }

                    reset_accumulation = true;

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
                            move_vec = ((scene*) main_scene.scene_buffer_ptr())->meta.cam.forward * move_speed;
                            break;
                        }
                        case KEYS::S: {
                            move_vec = -((scene*) main_scene.scene_buffer_ptr())->meta.cam.forward * move_speed;
                            break;
                        }
                        case KEYS::D: {
                            move_vec = ((scene*) main_scene.scene_buffer_ptr())->meta.cam.right * move_speed;
                            break;
                        }
                        case KEYS::A: {
                            move_vec = -((scene*) main_scene.scene_buffer_ptr())->meta.cam.right * move_speed;
                            break;
                        }
                        case KEYS::E: {
                            ((scene*) main_scene.scene_buffer_ptr())->meta.cam.rotate_y(0.001f * delta_time);
                            reset_accumulation = true;
                            break;
                        }
                        case KEYS::Q: {
                            ((scene*) main_scene.scene_buffer_ptr())->meta.cam.rotate_y(-0.001f * delta_time);
                            reset_accumulation = true;
                            break;
                        }
                        case KEYS::SPACE: {
                            move_vec = ((scene*) main_scene.scene_buffer_ptr())->meta.cam.up * move_speed;
                            break;
                        }
                        case KEYS::LCTRL: {
                            move_vec = -((scene*) main_scene.scene_buffer_ptr())->meta.cam.up * move_speed;
                            break;
                        }
                        default:
                            break;
                    }

                    ((scene*) main_scene.scene_buffer_ptr())->meta.cam.move(move_vec);

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
        ImGui::Text("frame count %u\n", ((scene*) main_scene.scene_buffer_ptr())->meta.sample_index);

        if (ImGui::Checkbox("depth of field", (bool*)&((scene*) main_scene.scene_buffer_ptr())->meta.enable_dof)) {
            reset_accumulation = true;
        }

        ImGui::SliderInt("max bounces", (int32_t*)&((scene*) main_scene.scene_buffer_ptr())->meta.max_bounce, 1, 250);
        ImGui::SliderInt("min bounces", (int32_t*)&((scene*) main_scene.scene_buffer_ptr())->meta.min_bounce, 1, ((scene*) main_scene.scene_buffer_ptr())->meta.max_bounce);
        if (ImGui::SliderInt("downscale factor", (int32_t*)&((scene*) main_scene.scene_buffer_ptr())->meta.downscale_factor, 1, 32)) {
            reset_accumulation = true;
        }

        if (ImGui::Checkbox("debug bvh", (bool*)&((scene*) main_scene.scene_buffer_ptr())->meta.debug_bvh)) {
            reset_accumulation = true;
        }

        ImGui::End();

        ImGui::EndFrame();

        // To start a frame capture, call StartFrameCapture.
        // You can specify NULL, NULL for the device to capture on if you have only one device and
        // either no windows at all or only one window, and it will capture from that device.
        // See the documentation below for a longer explanation
        // if(rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);

        //     float scale = (float)((scene*) main_scene.scene_buffer_ptr())->meta.downscale_factor;
        //     renderer.compute(width / scale, height / scale);

        //     ((scene*) main_scene.scene_buffer_ptr())->meta.sample_index++;

        //     renderer.ui();

        if (can_render) {
            raytracing_pass->set_ouput_texture(output_texture);
            raytracing_pass->set_constant(60, accumulation_texture);

            if (reset_accumulation) {
                ((scene*) main_scene.scene_buffer_ptr())->meta.sample_index = 0;
            }

            ((scene*) main_scene.scene_buffer_ptr())->meta.sample_index++;

            renderer.render();

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
