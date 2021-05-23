#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cassert>

#include "thirdparty/renderdoc.h"
#include "imgui.h"

#include "vk-renderer.hpp"
#include "window.hpp"
#include "utils.hpp"
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
    auto inputs = create_inputs(width, height);
    auto can_render = true;
    vkrenderer renderer { wnd, inputs };

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

                    ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->width = width;
                    ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->height = height;

                    io.DisplaySize.x = (float)width;
                    io.DisplaySize.y = (float)height;

                    if (event.width == 0 && event.height == 0) {
                        can_render = false;
                    } else {
                        can_render = true;
                        renderer.recreate_swapchain();
                        ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.set_aspect_ratio((float)event.width / (float)event.height);
                        ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->sample_index = 0;
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
                            move_vec = ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.forward.unit() * move_speed;
                            break;
                        }
                        case KEYS::S: {
                            move_vec = -((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.forward.unit() * move_speed;
                            break;
                        }
                        case KEYS::D: {
                            move_vec = ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.right.unit() * move_speed;
                            break;
                        }
                        case KEYS::A: {
                            move_vec = -((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.right.unit() * move_speed;
                            break;
                        }
                        case KEYS::E: {
                            ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.rotate_y(0.001f * delta_time);
                            reset_accumulation = true;
                            break;
                        }
                        case KEYS::Q: {
                            ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.rotate_y(-0.001f * delta_time);
                            reset_accumulation = true;
                            break;
                        }
                        case KEYS::SPACE: {
                            move_vec = ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.up.unit() * move_speed;
                            break;
                        }
                        case KEYS::CTRL: {
                            move_vec = -((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.up.unit() * move_speed;
                            break;
                        }
                        default:
                            break;
                    }

                    ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.move(move_vec);

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
        ImGui::Text("frame count %u\n", ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->sample_index);

        if (ImGui::Checkbox("depth of field", (bool*)&((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->enable_dof)) {
            reset_accumulation = true;
        }

        ImGui::SliderInt("bounces", (int32_t*)&((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->max_bounce, 2, 250);

        if (ImGui::Checkbox("debug bvh", (bool*)&((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->debug_bvh)) {
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
                ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->sample_index = 0;
                renderer.reset_accumulation();
            }

            renderer.compute(width, height);

            ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->sample_index++;

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
