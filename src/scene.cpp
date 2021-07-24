#include "scene.hpp"

#include "utils.hpp"

// Return the number of instantiated spheres
uint32_t scene::random_scene(sphere *world) {
    material ground_material = { { 0.5, 0.5, 0.5 } };
    ground_material.type = MATERIAL_TYPE::LAMBERTIAN;
    world[0] = sphere{ { 0, -1000, 0 }, ground_material, 1000 };

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

    material material4 = {};
    material4.albedo = color{ 0.0, 0.0, 0.0 };
    material4.emissive = color{ 1.0, 1.0, 1.0 } * 1000.0;
    material4.fuzz = 0.0;
    material4.type = MATERIAL_TYPE::EMISSIVE;
    world[world_sphere_index + 3] = { { 2, 1.5, 2 }, material4, 0.5 };

    return world_sphere_index + 4;
}

scene::scene(uint32_t width, uint32_t height)
    :width(width), height(height), spheres_count(random_scene(spheres)), 
        cam({ 13.0, 2.0, 3.0 }, { 0.0, 0.0, 0.0 }, 20.0, (float)width / (float)height, 0.1, 10) {
    bvh builder;
    builder.build(spheres, spheres_count);
    // builder.exporter();

    for (uint32_t idx = 0; idx < builder.nodes.size(); idx++) {
        nodes[idx] = builder.nodes[idx];
    }
}
