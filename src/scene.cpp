#include "scene.hpp"

#include <filesystem>

#include "utils.hpp"
#include "gltf.hpp"

// Return the number of instantiated spheres
// void scene::random_scene() {
//     material ground_material = { { 0.5, 0.5, 0.5 } };
//     ground_material.type = MATERIAL_TYPE::LAMBERTIAN;
//     spheres.emplace_back(vec3 { 0, -1000, 0 }, ground_material, 1000);
// 
//     int32_t spheres_count = 11;
//     for (int a = -spheres_count; a < spheres_count; a++) {
//         for (int b = -spheres_count; b < spheres_count; b++) {
//             auto choose_mat = randd();
//             point3 center(a + 0.9 * randd(), 0.2, b + 0.9 * randd());
// 
//             if ((center - point3{ 4, 0.2, 0 }).length() > 0.9) {
//                 material sphere_material = {};
// 
//                 if (choose_mat < 0.8) {
//                     // diffuse
//                     sphere_material.albedo = color::random() * color::random();
//                     sphere_material.type = MATERIAL_TYPE::LAMBERTIAN;
//                 } else if (choose_mat < 0.95) {
//                     // metal
//                     sphere_material.albedo = color::random(0.5, 1);
//                     sphere_material.fuzz = randd(0, 0.5);
//                     sphere_material.type = MATERIAL_TYPE::METAL;
//                 } else {
//                     // glass
//                     sphere_material.ior = 1.5;
//                     sphere_material.type = MATERIAL_TYPE::DIELECTRIC;
//                 }
// 
//                 spheres.emplace_back(center, sphere_material, 0.2);
//             }
//         }
//     }
// 
//     material material1 = {};
//     material1.ior = 1.5;
//     material1.type = MATERIAL_TYPE::DIELECTRIC;
//     spheres.emplace_back(vec3 { 0, 1, 0 }, material1, 1.0);
// 
//     material material2 = {};
//     material2.albedo = color{ 0.4, 0.2, 0.1 };
//     material2.type = MATERIAL_TYPE::LAMBERTIAN;
//     spheres.emplace_back(vec3 { -4, 1, 0 }, material2, 1.0);
// 
//     material material3 = {};
//     material3.albedo = color{ 0.7, 0.6, 0.5 };
//     material3.fuzz = 0.0;
//     material3.type = MATERIAL_TYPE::METAL;
//     spheres.emplace_back(vec3 { 4, 1, 0 }, material3, 1.0);
// 
//     material material4 = {};
//     material4.albedo = color{ 0.0, 0.0, 0.0 };
//     material4.emissive = color{ 1.0, 1.0, 1.0 } * 1000.0;
//     material4.fuzz = 0.0;
//     material4.type = MATERIAL_TYPE::EMISSIVE;
//     spheres.emplace_back(vec3 { 2, 1.5, 2 }, material4, 0.5);
// 
//     spheres_count = spheres.size();
// }

scene::scene(camera cam, uint32_t width, uint32_t height)
    :meta(cam, width, height){

    auto sponza_path = std::filesystem::path("../models/sponza");
    auto sponza_filename = std::string("Sponza.gltf");
    auto mesh = gltf::load(sponza_path, sponza_filename);

    auto node = mesh.nodes[0];
    for (auto& mesh_part: node) {
        for (size_t i = 0; i < mesh_part.indices.size(); i += 3) {
            auto i1 = mesh_part.indices[i];
            auto i2 = mesh_part.indices[i + 1];
            auto i3 = mesh_part.indices[i + 2];

            auto v1 = vertex { .position = mesh_part.positions[i1] };
            auto v2 = vertex { .position = mesh_part.positions[i2] };
            auto v3 = vertex { .position = mesh_part.positions[i3] };

            triangles.emplace_back(v1, v2, v3);
        }
    }

    // random_scene();

    bvh builder(triangles, nodes);
    // bvh builder(spheres, nodes);
    // builder.exporter();

    std::memcpy(nodes.data(), builder.nodes.data(), sizeof(nodes[0]) * nodes.size());
}
