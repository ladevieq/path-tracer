#include "scene.hpp"

#include <filesystem>
#include <queue>

#include "vk-renderer.hpp"
#include "gltf.hpp"
#include "bvh.hpp"
#include "material.hpp"

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


scene::metadata::metadata(const camera &cam, uint32_t width, uint32_t height)
    : cam(cam), width(width), height(height) {}

scene::scene(const camera& cam, uint32_t width, uint32_t height)
    :meta(cam, width, height){

    // Geometry & BVH
    std::vector<uint32_t>   indices;
    std::vector<float>      positions;
    std::vector<float>      normals;
    std::vector<float>      uvs;
    std::vector<material>   materials;
    std::vector<packed_bvh_node> packed_nodes;

    // auto root_node = gltf::load("../models/BistroInterior", "BistroInterior.gltf");
    auto gltf_model = gltf("../models/sponza/Sponza.gltf");

    std::vector<triangle> triangles;
    size_t triangles_offset = 0;
    size_t vertices_offset = 0;
    std::queue<node> nodes_to_load {};
    nodes_to_load.push(gltf_model.root_node);

    while(!nodes_to_load.empty()) {
        auto &node = nodes_to_load.front();

        if (node.mesh != nullptr) {
            const auto* mesh = node.mesh;
            const auto& mesh_positions = mesh->get_attribute(ATTRIBUTE_TYPE::POSITION);
            const auto& mesh_normals = mesh->get_attribute(ATTRIBUTE_TYPE::NORMAL);
            const auto& mesh_uvs_0 = mesh->get_attribute(ATTRIBUTE_TYPE::UV_0);
            const auto& mesh_indices = mesh->get_indices();

            indices.resize(indices.size()      + mesh_indices.size());
            positions.resize(vertices_offset   + mesh_positions.size());
            normals.resize(vertices_offset     + mesh_normals.size());
            uvs.resize(vertices_offset         + mesh_uvs_0.size());

            std::memcpy(
               positions.data() + vertices_offset * Mesh::attribute_components(ATTRIBUTE_TYPE::POSITION),
               mesh_positions.data(),
               mesh_positions.size() * sizeof(float)
            );

            std::memcpy(
               normals.data() + vertices_offset * Mesh::attribute_components(ATTRIBUTE_TYPE::NORMAL),
               mesh_normals.data(),
               mesh_normals.size() * sizeof(float)
            );

            std::memcpy(
                uvs.data() + vertices_offset * Mesh::attribute_components(ATTRIBUTE_TYPE::UV_0),
                mesh_uvs_0.data(),
                mesh_uvs_0.size() * sizeof(float)
            );

            auto index_offset { 0U };
            // Add offset to indices in order to have absolute indices
            for (const auto& submesh: mesh->get_submeshes()) {
                auto vertex_offset = vertices_offset + submesh.vertex_offset;

                for (size_t element_index { 0U }; element_index < submesh.element_count; element_index++, index_offset += 3) {
                    size_t submesh_level_index_1 = mesh_indices[index_offset];
                    size_t submesh_level_index_2 = mesh_indices[index_offset + 1];
                    size_t submesh_level_index_3 = mesh_indices[index_offset + 2];

                    size_t mesh_level_index_1 = (submesh.vertex_offset + submesh_level_index_1) * 3;
                    size_t mesh_level_index_2 = (submesh.vertex_offset + submesh_level_index_2) * 3;
                    size_t mesh_level_index_3 = (submesh.vertex_offset + submesh_level_index_3) * 3;

                    triangles.emplace_back(
                        vec3 { mesh_positions[mesh_level_index_1], mesh_positions[mesh_level_index_1 + 1], mesh_positions[mesh_level_index_1 + 2] },
                        vec3 { mesh_positions[mesh_level_index_2], mesh_positions[mesh_level_index_2 + 1], mesh_positions[mesh_level_index_2 + 2] },
                        vec3 { mesh_positions[mesh_level_index_3], mesh_positions[mesh_level_index_3 + 1], mesh_positions[mesh_level_index_3 + 2] }
                    );

                    auto triangle_offset = triangles_offset * 3 + index_offset;

                    indices[triangle_offset]        = (submesh_level_index_1 + vertex_offset); // | (0xff000000 & (materials.size() << 8));
                    indices[triangle_offset + 1]    = (submesh_level_index_2 + vertex_offset); // | (0xff000000 & (materials.size() << 16));
                    indices[triangle_offset + 2]    = (submesh_level_index_3 + vertex_offset); // | (0xff000000 & (materials.size() << 24));

                    // materials.push_back(submesh.mat);
                }
            }

            triangles_offset += mesh->triangle_count();
            vertices_offset += mesh->vertices_count();
        }

        for (auto &new_node: node.children) {
            nodes_to_load.push(new_node);
        }

        nodes_to_load.pop();
    }

    // random_scene();

    bvh builder(triangles, packed_nodes);
    // bvh builder(spheres, packed_nodes);

    scene_buffer = vkrenderer::create_buffer(sizeof(meta) * vkrenderer::virtual_frames_count);
    scene_buffer->write(&meta, 0, sizeof(meta));
    scene_buffer->write(&meta, sizeof(meta), sizeof(meta));
    scene_buffer->write(&meta, sizeof(meta) * 2, sizeof(meta));

    indices_buffer = vkrenderer::create_buffer(indices.size() * sizeof(indices[0]));
    indices_buffer->write(indices.data(), 0, indices.size() * sizeof(indices[0]));

    positions_buffer = vkrenderer::create_buffer(positions.size() * sizeof(positions[0]));
    positions_buffer->write(positions.data(), 0, positions.size() * sizeof(positions[0]));

    normals_buffer = vkrenderer::create_buffer(normals.size() * sizeof(normals[0]));
    normals_buffer->write(normals.data(), 0, normals.size() * sizeof(normals[0]));

    uvs_buffer = vkrenderer::create_buffer(uvs.size() * sizeof(uvs[0]));
    uvs_buffer->write(uvs.data(), 0, uvs.size() * sizeof(uvs[0]));

    bvh_buffer = vkrenderer::create_buffer(packed_nodes.size() * sizeof(packed_nodes[0]));
    bvh_buffer->write(packed_nodes.data(), 0, packed_nodes.size() * sizeof(packed_nodes[0]));

    // materials_buffer = vkrenderer::create_buffer(materials.size() * sizeof(materials[0]));
    // materials_buffer->write(materials.data(), 0, materials.size() * sizeof(materials[0]));
}
