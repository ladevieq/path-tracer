#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cassert>

#include "thirdparty/renderdoc.h"

#include "vk-renderer.hpp"
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
    // const float aspect_ratio = 1;
    const uint32_t width = 400;
    const uint32_t height = width / aspect_ratio;

    window wnd { width , height };

    auto inputs = create_inputs(width, height);
    auto canRender = true;
    vkrenderer renderer { wnd, inputs };

    while(wnd.isOpen) {
        wnd.poll_events();

        for (auto event: wnd.events) {
            switch(event.type) {
                case EVENT_TYPES::RESIZE: {
                    renderer.mapped_data->width = event.width;
                    renderer.mapped_data->height = event.height;

                    if (event.width == 0 && event.height == 0) {
                        canRender = false;
                    } else {
                        canRender = true;
                        renderer.recreate_swapchain();
                        renderer.mapped_data->cam.set_aspect_ratio((float)event.width / (float)event.height);
                    }
                    break;
                }
                case EVENT_TYPES::KEY_PRESS: {
                    switch((uint8_t)event.keycode) {
                        case('w'): {
                            renderer.mapped_data->cam.move(renderer.mapped_data->cam.forward * 0.1);
                            break;
                        }
                        case('s'): {
                            renderer.mapped_data->cam.move(-renderer.mapped_data->cam.forward * 0.1);
                            break;
                        }
                        default:
                            std::cerr << (uint8_t)event.keycode << std::endl;
                    }
                }
                default: {
                }
            }
        }

        wnd.events.clear();

        // To start a frame capture, call StartFrameCapture.
        // You can specify NULL, NULL for the device to capture on if you have only one device and
        // either no windows at all or only one window, and it will capture from that device.
        // See the documentation below for a longer explanation
        if(rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);

        if (canRender) {
            renderer.compute();
        }

        // stop the capture
        if(rdoc_api) rdoc_api->EndFrameCapture(NULL, NULL);
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
