#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cassert>

#include "thirdparty/renderdoc.h"

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
    const uint32_t samples_per_pixel = 25;

    window wnd { width , height };

    float delta_time = 0.f;
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    auto inputs = create_inputs(width, height, samples_per_pixel);
    auto canRender = true;
    vkrenderer renderer { wnd, inputs };


    while(wnd.isOpen) {
        end = std::chrono::high_resolution_clock::now();

        delta_time = std::chrono::duration<float, std::milli>(end - start).count();

        start = std::chrono::high_resolution_clock::now();

        wnd.poll_events();

        for (auto event: wnd.events) {
            switch(event.type) {
                case EVENT_TYPES::RESIZE: {
                    width = event.width;
                    height = event.height;
                    ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->width = width;
                    ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->height = height;

                    if (event.width == 0 && event.height == 0) {
                        canRender = false;
                    } else {
                        canRender = true;
                        renderer.recreate_swapchain();
                        ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.set_aspect_ratio((float)event.width / (float)event.height);
                        ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->sample_index = 0;
                    }
                    break;
                }
                case EVENT_TYPES::KEY_PRESS: {
                    auto move_speed = 0.01f * delta_time;
                    if (wnd.keyboard[KEYS::LSHIFT] || wnd.keyboard[KEYS::RSHIFT]) {
                        move_speed *= 2.f;
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
                        default:
                            break;
                    }

                    ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->cam.move(move_vec);

                    if (!move_vec.near_zero()) {
                        ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->sample_index = 0;
                    }

                    break;
                }
                default:
                    break;
            }
        }

        wnd.events.clear();

        // To start a frame capture, call StartFrameCapture.
        // You can specify NULL, NULL for the device to capture on if you have only one device and
        // either no windows at all or only one window, and it will capture from that device.
        // See the documentation below for a longer explanation
        // if(rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);

        if (canRender) {
            renderer.begin_frame();
            renderer.ui();

            // if (((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->sample_index < samples_per_pixel) {
            //     bool clear = ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->sample_index == 0;
            //     renderer.compute(width, height, clear);

            //     ((input_data*) renderer.compute_shader_buffer.alloc_info.pMappedData)->sample_index++;
            // }

            renderer.finish_frame();
        }

        // stop the capture
        // if(rdoc_api) rdoc_api->EndFrameCapture(NULL, NULL);
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
