#include "scene.hpp"

#include <filesystem>
#include <queue>

#include "utils.hpp"
#include "gltf.hpp"

// void scene::random_scene() {
//     material ground_material = { { 0.5, 0.5, 0.5 } };
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
//                     sphere_material.base_color = color::random() * color::random();
//                 } else if (choose_mat < 0.95) {
//                     // metal
//                     sphere_material.base_color = color::random(0.5, 1);
//                     sphere_material.metalness = randd(0, 0.5);
//                     sphere_material.roughness = 0.f;
//                 } else {
//                     // glass
//                     // sphere_material.ior = 1.5;
//                     sphere_material.metalness = 1.f;
//                     sphere_material.roughness = 0.f;
//                 }
// 
//                 spheres.emplace_back(center, sphere_material, 0.2);
//             }
//         }
//     }
// 
//     material material1 = {};
//     // material1.ior = 1.5;
//     spheres.emplace_back(vec3 { 0, 1, 0 }, material1, 1.0);
// 
//     material material2 = {};
//     material2.base_color = color{ 0.4, 0.2, 0.1 };
//     spheres.emplace_back(vec3 { -4, 1, 0 }, material2, 1.0);
// 
//     material material3 = {};
//     material3.base_color = color{ 0.7, 0.6, 0.5 };
//     material3.roughness = 0.0;
//     spheres.emplace_back(vec3 { 4, 1, 0 }, material3, 1.0);
// 
//     spheres_count = spheres.size();
// }

scene::scene(camera cam, uint32_t width, uint32_t height)
    :meta(cam, width, height){

    auto bistro_interior_path = std::filesystem::path("../models/BistroInterior");
    auto bistro_interior_filename = std::string("BistroInterior.gltf");
    auto root_node = gltf::load(bistro_interior_path, bistro_interior_filename);

    // auto sponza_path = std::filesystem::path("../models/sponza");
    // auto sponza_filename = std::string("Sponza.gltf");
    // auto root_node = gltf::load(sponza_path, sponza_filename);

    std::queue<node> nodes_to_load {};
    nodes_to_load.push(root_node);

    while(nodes_to_load.size() != 0) {
        auto &node = nodes_to_load.front();

        if (node.mesh) {
            auto &mesh = node.mesh.value();
            for (auto& mesh_part: mesh.parts) {
                for (size_t i = 0; i < mesh_part.indices.size(); i += 3) {
                    auto i1 = mesh_part.indices[i];
                    auto i2 = mesh_part.indices[i + 1];
                    auto i3 = mesh_part.indices[i + 2];

                    auto v1 = vertex { .position = mesh_part.positions[i1], .normal = mesh_part.normals[i1] };
                    auto v2 = vertex { .position = mesh_part.positions[i2], .normal = mesh_part.normals[i2] };
                    auto v3 = vertex { .position = mesh_part.positions[i3], .normal = mesh_part.normals[i3] };

                    triangles.emplace_back(v1, v2, v3);
                }
            }
        }

        for (auto &node: node.children) {
            nodes_to_load.push(node);
        }

        nodes_to_load.pop();
    }

    // random_scene();

    bvh builder(triangles, packed_nodes);
    // bvh builder(spheres, packed_nodes);
}
