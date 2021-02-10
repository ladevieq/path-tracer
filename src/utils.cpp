#include "utils.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>

#include "material.hpp"

std::vector<uint8_t> get_shader_code(const char* path) {
    FILE* f = fopen(path, "rb");

    if (f == nullptr) {
        std::cerr << "Cannot open compute shader file !" << std::endl;
        return {};
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        return {};
    }

    std::vector<uint8_t> shader_code(ftell(f));

    if (fseek(f, 0, SEEK_SET) != 0) {
        return {};
    }

    if (fread(shader_code.data(), sizeof(uint8_t), shader_code.size(), f) != shader_code.size()) {
        std::cerr << "Reading shader code failed" << std::endl;
        return {};
    }

    return shader_code;
}

color ground_color { 1.0, 1.0, 1.0 };
color sky_color { 0.5, 0.7, 1.0 };

void random_scene(sphere *world) {
    material ground_material = { { 0.5, 0.5, 0.5 } };
    ground_material.type = MATERIAL_TYPE::LAMBERTIAN;
    world[0] = sphere{ { 0,-1000,0 }, ground_material, 1000 };

    uint32_t world_sphere_index = 1;

    int32_t spheres_count = 11;
    for (int a = -spheres_count; a < spheres_count; a++) {
        for (int b = -spheres_count; b < spheres_count; b++) {
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

struct input_data create_inputs(uint32_t width, uint32_t height, uint32_t samples_per_pixel) {
    // Image dimensions
    const float aspect_ratio = (float)width / (float) height;

    const uint32_t max_depth = 20;

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
        .height = height,

        .sample_index = 0,
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

    return inputs;
}
