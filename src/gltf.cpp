#include "gltf.hpp"

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <execution>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "vk-renderer.hpp"
#include "utils.hpp"

gltf::gltf(const std::filesystem::path &filepath) {
    auto parent_path = filepath.parent_path();
    std::fstream f { filepath };

    f >> gltf_json;

    auto& scene = gltf_json["scenes"][0];

    auto &gltf_buffers = gltf_json["buffers"];
    auto buffer_count = gltf_buffers.size();
    buffers.resize(gltf_buffers.size());
    for (auto buffer_index { 0U }; buffer_index < buffer_count; buffer_index++) {
        auto buffer_path = parent_path / gltf_buffers[buffer_index]["uri"].get<std::string>();
        buffers[buffer_index] = read_file(buffer_path.string().c_str());
    }

    load_textures(parent_path);
    load_materials();

    load_meshes();

    auto& root_children = scene["nodes"];
    uint32_t root_children_count = scene["nodes"].size();
    root_node.children.resize(root_children_count);

    for (size_t child_index = 0; child_index < root_children_count; child_index++) {
        load_node(root_children[child_index].get<uint32_t>(), root_node.children[child_index]);
    }

    f.close();
}

void gltf::load_node(uint32_t index, node &parent) {
    const auto &gltf_node = gltf_json["nodes"][index];

    if (gltf_node.contains("children")) {
        const auto &children = gltf_node["children"];
        uint32_t children_count = gltf_node["children"];
        parent.children.resize(children_count);

        for (size_t child_index = 0; child_index < children_count; child_index++) {
            load_node(children[child_index].get<uint32_t>(), parent.children[child_index]);
        }
    }

    if (gltf_node.contains("mesh")) {
        parent.mesh = &meshes[gltf_node["mesh"].get<uint32_t>()];
    }
}

void gltf::load_meshes() {
    const auto &gltf_meshes = gltf_json["meshes"];
    auto meshes_count = gltf_meshes.size();
    meshes.resize(meshes_count);

    for (size_t mesh_index = 0; mesh_index < meshes_count; mesh_index++) {
        auto& mesh = meshes[mesh_index];
        const auto &primitives = gltf_meshes[mesh_index]["primitives"];

        for (const auto &primitive : primitives) {
            auto& material = materials[primitive["material"].get<uint32_t>()];
            mesh.add_submesh(load_primitive(primitive), material);
        }

        Mesh::register_mesh(mesh);
    }
}

Mesh::submesh gltf::load_primitive(const json &primitive) {
    Mesh::submesh submesh;

    submesh.indices = load_attribute(primitive["indices"]);
    submesh.attributes[ATTRIBUTE_TYPE::POSITION] = load_attribute(primitive["attributes"]["POSITION"]);

    if (primitive["attributes"].contains("NORMAL")) {
        submesh.attributes[ATTRIBUTE_TYPE::NORMAL] = load_attribute(primitive["attributes"]["NORMAL"]);
    }

    if (primitive["attributes"].contains("TEXCOORD_0")) {
        submesh.attributes[ATTRIBUTE_TYPE::UV_0] = load_attribute(primitive["attributes"]["TEXCOORD_0"]);
    }

    submesh.index_count = gltf_json["accessors"][primitive["indices"].get<uint32_t>()]["count"].get<uint32_t>();
    submesh.vertex_count = gltf_json["accessors"][primitive["attributes"]["POSITION"].get<uint32_t>()]["count"].get<uint32_t>();

    return submesh;
}

Mesh::attribute gltf::load_attribute(uint32_t accessor_index) {
    auto view_index = gltf_json["accessors"][accessor_index]["bufferView"].get<uint32_t>();
    const auto &view = gltf_json["bufferViews"][view_index];
    auto offset = view["byteOffset"].get<off_t>();
    auto &buffer = buffers[view["buffer"].get<uint32_t>()];

    return {
        &buffer[offset],
        view["byteLength"].get<size_t>()
    };
}

void gltf::load_textures(const std::filesystem::path &path) {
    const auto &gltf_images = gltf_json["images"];
    auto images_count = gltf_images.size();
    std::vector<raw_image> images{images_count};

    std::vector<size_t> v(images_count);
    std::iota(v.begin(), v.end(), 0);

    std::for_each(std::execution::par, v.begin(), v.end(), [&](const size_t image_index) {
        const auto &gltf_image = gltf_images[image_index];
        auto filepath = (path / gltf_image["uri"].get<std::string>()).string();
        auto& image = images[image_index];
        image.data = stbi_load(filepath.c_str(), &image.width, &image.height, &image.channels, 4);
    });

    // const auto &gltf_samplers = gltf_json["samplers"];
    // auto samplers_count = gltf_samplers.size();
    // std::vector<Sampler *> samplers{samplers_count};

    Sampler* sampler = vkrenderer::create_sampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    // for (size_t sampler_index = 0; sampler_index < samplers_count; sampler_index++) {
    //     samplers[sampler_index] = vkrenderer::create_sampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    // }

    const auto &gltf_textures = gltf_json["textures"];
    auto textures_count = gltf_textures.size();
    textures.resize(textures_count);

    for (size_t texture_index = 0; texture_index < textures_count; texture_index++) {
        auto gltf_texture = gltf_textures[texture_index];

        auto& image = images[gltf_texture["source"].get<uint32_t>()];
        // if (gltf_texture.contains("sampler")) {
        //     auto sampler_index = gltf_texture["sampler"].get<uint32_t>();
        //     sampler = samplers[sampler_index];
        // }

        auto *texture = vkrenderer::create_2d_texture(static_cast<uint32_t>(image.width), static_cast<uint32_t>(image.height), VK_FORMAT_R8G8B8A8_UNORM, sampler);

        texture->update(image.data);

        textures[texture_index] = texture;
    }
}

void gltf::load_materials() {
    const auto &gltf_materials = gltf_json["materials"];
    auto materials_count = gltf_materials.size();
    materials.resize(materials_count);

    for (size_t material_index = 0; material_index < materials_count; material_index++) {
        const auto &gltf_material = gltf_materials[material_index];
        auto &material = materials[material_index];

        if (gltf_material.contains("pbrMetallicRoughness")) {
            const auto &pbr_params = gltf_material["pbrMetallicRoughness"];

            if (pbr_params.contains("baseColorFactor")) {
                const auto &base_color = pbr_params["baseColorFactor"];
                material.base_color.v[0] = base_color[0].get<float>();
                material.base_color.v[1] = base_color[1].get<float>();
                material.base_color.v[2] = base_color[2].get<float>();
                material.base_color.v[3] = base_color[3].get<float>();
            } else {
                material.base_color = vec3(1.f);
            }

            if (pbr_params.contains("baseColorTexture")) {
                auto base_color_index = pbr_params["baseColorTexture"]["index"].get<uint32_t>();
                material.base_color_texture = textures[base_color_index];
            }

            if (pbr_params.contains("metallicFactor")) {
                material.metalness = pbr_params["metallicFactor"].get<float>();
            } else {
                material.metalness = 1.0;
            }

            if (pbr_params.contains("roughnessFactor")) {
                material.roughness = pbr_params["roughnessFactor"].get<float>();
            } else {
                material.roughness = 1.0;
            }

            if (pbr_params.contains("metallicRoughnessTexture")) {
                auto metallic_roughness_index = pbr_params["metallicRoughnessTexture"]["index"].get<uint32_t>();
                material.metallic_roughness_texture = textures[metallic_roughness_index];
            }
        }
    }
}
