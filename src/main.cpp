#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cassert>


#if defined(LINUX)
#include <unistd.h>
#include <dlfcn.h>
#elif defined(WINDOWS)
#include <Windows.h>
#endif

#include "vk-renderer.hpp"
#include "thirdparty/renderdoc.h"

#include "vec3.hpp"
#include "material.hpp"
#include "sphere.hpp"
#include "camera.hpp"
#include "utils.hpp"

color ground_color { 1.0, 1.0, 1.0 };
color sky_color { 0.5, 0.7, 1.0 };

void random_scene(sphere *world) {
    material ground_material = { { 0.5, 0.5, 0.5 } };
    ground_material.type = MATERIAL_TYPE::LAMBERTIAN;
    world[0] = sphere{ { 0,-1000,0 }, ground_material, 1000 };

    uint32_t world_sphere_index = 1;

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = randd();
            point3 center(a + 0.9 * randd(), 0.2, b + 0.9 * randd());

            if ((center - point3{ 4, 0.2, 0 }).length() > 0.9) {
                material sphere_material = {};

                if (choose_mat < 0.8) {
                    // diffuse
                    sphere_material.albedo = color::random() * color::random();
                    sphere_material.type = MATERIAL_TYPE::LAMBERTIAN;
                } else if (choose_mat < 0.95) {
                    // metal
                    sphere_material.albedo = color::random(0.5, 1);
                    sphere_material.fuzz = randd(0, 0.5);
                    sphere_material.type = MATERIAL_TYPE::METAL;
                } else {
                    // glass
                    sphere_material.ior = 1.5;
                    sphere_material.type = MATERIAL_TYPE::DIELECTRIC;
                }

                world[world_sphere_index] = { center, sphere_material, 0.2 };
                world_sphere_index++;
            }
        }
    }

    material material1 = {};
    material1.ior = 1.5;
    material1.type = MATERIAL_TYPE::DIELECTRIC;
    world[world_sphere_index] = { { 0, 1, 0 }, material1, 1.0 };

    material material2 = {};
    material2.albedo = color{ 0.4, 0.2, 0.1 };
    material2.type = MATERIAL_TYPE::LAMBERTIAN;
    world[world_sphere_index + 1] = { { -4, 1, 0 }, material2, 1.0 };

    material material3 = {};
    material3.albedo = color{ 0.7, 0.6, 0.5 };
    material3.fuzz = 0.0;
    material3.type = MATERIAL_TYPE::METAL;
    world[world_sphere_index + 2] = { { 4, 1, 0 }, material3, 1.0 };
}

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
    // const float aspect_ratio = 16.0 / 9.0;
    const float aspect_ratio = 1;
    const uint32_t width = 400;
    const uint32_t height = width / aspect_ratio;

    window wnd { width , height };
    vkrenderer renderer { wnd };

    const uint32_t samples_per_pixel = 25;
    const uint32_t max_depth = 25;

    const vec3 camera_position{ 13.0, 2.0, 3.0 };
    const vec3 camera_target{ 0.0, 0.0, 0.0 };
    const float aperture = 0.1;
    const float distance_to_focus = 10;
    camera camera { camera_position, camera_target, 20.0, aspect_ratio, aperture, distance_to_focus };

    struct input_data inputs = {
        .sky_color = sky_color,
        .ground_color = ground_color,

        .cam = camera,

        .spheres = {},

        .max_bounce = max_depth,
        .samples_per_pixel = samples_per_pixel,
        .width = width,
        .height = height
    };

    // Random numbers pools
    for (size_t rand_number_index = 0; rand_number_index < 1000; rand_number_index += 2) {
        inputs.random_offset[rand_number_index] = randd();
        inputs.random_offset[rand_number_index + 1] = randd();
    }

    for (size_t rand_number_index = 0; rand_number_index < 1000; rand_number_index += 2) {
        vec3 random_unit_disk = random_in_unit_disk();
        inputs.random_disk[rand_number_index] = random_unit_disk.x;
        inputs.random_disk[rand_number_index + 1] = random_unit_disk.y;
    }

    for (size_t rand_number_index = 0; rand_number_index < 1000; rand_number_index++) {
        inputs.random_in_sphere[rand_number_index] = random_in_unit_sphere();
    }

    // World hittable objects
    random_scene(inputs.spheres);

    // PPM format header
    // std::cout << "P3\n" << width << '\n' << height << "\n255" << std::endl;

    // std::cerr << "Generating image" << std::endl;

    // To start a frame capture, call StartFrameCapture.
    // You can specify NULL, NULL for the device to capture on if you have only one device and
    // either no windows at all or only one window, and it will capture from that device.
    // See the documentation below for a longer explanation
    if(rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);

    auto start = std::chrono::high_resolution_clock::now();
    renderer.compute(inputs, width, height);
    auto end = std::chrono::high_resolution_clock::now();

    std::cerr << "Image generation took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    // stop the capture
    if(rdoc_api) rdoc_api->EndFrameCapture(NULL, NULL);

    //-------------------------
    // GPU path tracer
    //-------------------------
    // auto output_image = renderer.output_image();
    // for (size_t index = 0; index < height * width; index++) {
    //     write_color(std::cout, output_image[index]);
    // }

    std::cerr << "Done !" << std::endl;

    return 0;
}
