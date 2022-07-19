#include "gltf.hpp"

#include <fstream>
#include <cstring>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "vk-renderer.hpp"
#include "utils.hpp"

node gltf::load(std::filesystem::path &path, std::string &filename, vkrenderer &renderer) {
    std::fstream f { path / filename };
    json gltf_json;

    f >> gltf_json;

    auto& scene = gltf_json["scenes"][0];

    auto &buffers = gltf_json["buffers"];
    auto buffers_content = std::vector<std::vector<uint8_t>>(buffers.size());
    for (size_t buffer_index{0}; buffer_index < buffers.size(); buffer_index++) {
        auto& buffer = buffers[buffer_index];
        auto buffer_path = path / buffer["uri"].get<std::string>();
        buffers_content[buffer_index] = read_file(buffer_path.string().c_str());
    }

    auto textures = load_textures(gltf_json, path, renderer);
    auto materials = load_materials(gltf_json, textures);

    auto meshes = load_meshes(gltf_json, buffers_content, materials);

    node root_node;
    auto& root_children = scene["nodes"];
    uint32_t root_children_count = scene["nodes"].size();
    root_node.children.resize(root_children_count);

    for (size_t child_index = 0; child_index < root_children_count; child_index++) {
        load_node(gltf_json, root_children[child_index].get<uint32_t>(), root_node.children[child_index], meshes);
    }

    f.close();

    return root_node;
}

void gltf::load_node(json &gltf_json, uint32_t index, node &parent, std::vector<mesh> &meshes) {
    auto &gltf_node = gltf_json["nodes"][index];

    if (gltf_node.contains("children")) {
        auto &children = gltf_node["children"];
        uint32_t children_count = gltf_node["children"];
        parent.children.resize(children_count);

        for (size_t child_index = 0; child_index < children_count; child_index++) {
            load_node(gltf_json, children[child_index].get<uint32_t>(), parent.children[child_index], meshes);
        }
    }

    if (gltf_node.contains("mesh")) { parent.mesh = std::make_optional(meshes[gltf_node["mesh"].get<uint32_t>()]); }
}

std::vector<mesh> gltf::load_meshes(
    json &gltf_json, std::vector<std::vector<uint8_t>> &buffers_content, std::vector<material> &materials) {
    auto &gltf_meshes = gltf_json["meshes"];
    auto meshes_count = gltf_meshes.size();
    std::vector<mesh> meshes{meshes_count};

    for (size_t mesh_index = 0; mesh_index < meshes_count; mesh_index++) {
        auto &gltf_mesh = gltf_meshes[mesh_index];
        auto &primitives = gltf_mesh["primitives"];
        auto &mesh = meshes[mesh_index];

        for (auto &primitive : primitives) {
            auto part = load_primitive(gltf_json, primitive, buffers_content, materials);
            mesh.vertices_count += part.vertices_count;
            mesh.triangles_count += part.triangles_count;
            mesh.parts.emplace_back(std::move(part));
        }
    }

    return meshes;
}

std::vector<texture> gltf::load_textures(json &gltf_json, std::filesystem::path &path, vkrenderer &renderer) {
    auto &gltf_images = gltf_json["images"];
    auto images_count = gltf_images.size();
    std::vector<Texture *> images{images_count};

    for (size_t image_index = 0; image_index < images_count; image_index++) {
        auto &gltf_image = gltf_images[image_index];
        int x, y, n;
        auto filepath = (path / gltf_image["uri"].get<std::string>()).string();
        unsigned char *data = stbi_load(filepath.c_str(), &x, &y, &n, 4);
        auto *img = renderer.create_2d_texture(static_cast<uint32_t>(x), static_cast<uint32_t>(y), VK_FORMAT_R8G8B8A8_UNORM);

        renderer.update_image(img, data);

        stbi_image_free(data);

        images[image_index] = img;
    }

    auto &gltf_samplers = gltf_json["samplers"];
    auto samplers_count = gltf_samplers.size();
    std::vector<Sampler *> samplers{samplers_count};

    for (size_t sampler_index = 0; sampler_index < samplers_count; sampler_index++) {
        samplers[sampler_index] = renderer.create_sampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    }

    auto& api = renderer.api;
    auto &gltf_textures = gltf_json["textures"];
    auto textures_count = gltf_textures.size();
    std::vector<texture> textures{textures_count};

    for (size_t texture_index = 0; texture_index < textures_count; texture_index++) {
        auto gltf_texture = gltf_textures[texture_index];

        auto image_index = gltf_texture["source"].get<uint32_t>();
        auto &image = images[image_index];

        uint32_t image_id = api.get_image(image->device_image).bindless_sampled_index;
        uint32_t sampler_id = 0;
        if (gltf_texture.contains("sampler")) {
            auto sampler_index = gltf_texture["sampler"].get<uint32_t>();
            auto &sampler = samplers[sampler_index];
            sampler_id = api.get_sampler(sampler->device_sampler).bindless_index;
        }

        textures[texture_index] =
            texture{.image_id = image_id, .sampler_id = sampler_id};
    }

    return textures;
}

std::vector<material> gltf::load_materials(json &gltf_json, std::vector<texture> &textures) {
    auto &gltf_materials = gltf_json["materials"];
    auto materials_count = gltf_materials.size();
    std::vector<material> materials{materials_count};

    for (size_t material_index = 0; material_index < materials_count; material_index++) {
        auto &gltf_material = gltf_materials[material_index];
        auto &material = materials[material_index];

        if (gltf_material.contains("pbrMetallicRoughness")) {
            auto &pbr_params = gltf_material["pbrMetallicRoughness"];

            if (pbr_params.contains("baseColorFactor")) {
                auto &base_color = pbr_params["baseColorFactor"];
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

    return materials;
}

mesh_part gltf::load_primitive(json &gltf_json, json &primitive, std::vector<std::vector<uint8_t>> &buffers_content,
    std::vector<material> &materials) {
    auto part = mesh_part();
    auto &accessors = gltf_json["accessors"];

    {
        auto indices_accessor_index = primitive["indices"].get<uint32_t>();
        auto indices_view_index = accessors[indices_accessor_index]["bufferView"].get<uint32_t>();
        auto &indices_view = gltf_json["bufferViews"][indices_view_index];

        auto indices_buffer_index = indices_view["buffer"].get<uint32_t>();
        auto indices_offset = indices_view["byteOffset"].get<size_t>();
        auto indices_length = indices_view["byteLength"].get<size_t>();

        auto &indices_buffer = buffers_content[indices_buffer_index];
        auto *indices_start = (uint16_t *)(indices_buffer.data() + indices_offset);

        part.indices.resize(indices_length / sizeof(uint16_t));
        std::memcpy(part.indices.data(), indices_start, indices_length);
    }

    {
        auto positions_accessor_index = primitive["attributes"]["POSITION"].get<uint32_t>();
        auto positions_view_index = accessors[positions_accessor_index]["bufferView"].get<uint32_t>();
        auto &positions_view = gltf_json["bufferViews"][positions_view_index];

        auto positions_buffer_index = positions_view["buffer"].get<uint32_t>();
        auto positions_offset = positions_view["byteOffset"].get<size_t>();
        auto positions_length = positions_view["byteLength"].get<size_t>();

        auto &positions_buffer = buffers_content[positions_buffer_index];
        auto *positions_start = (float *)(positions_buffer.data() + positions_offset);

        part.positions.resize(positions_length / sizeof(float));
        std::memcpy(part.positions.data(), positions_start, positions_length);
    }

    {
        auto normals_accessor_index = primitive["attributes"]["NORMAL"].get<uint32_t>();
        auto normals_view_index = accessors[normals_accessor_index]["bufferView"].get<uint32_t>();
        auto &normals_view = gltf_json["bufferViews"][normals_view_index];

        auto normals_buffer_index = normals_view["buffer"].get<uint32_t>();
        auto normals_offset = normals_view["byteOffset"].get<size_t>();
        auto normals_length = normals_view["byteLength"].get<size_t>();

        auto &normals_buffer = buffers_content[normals_buffer_index];
        auto *normals_start = (float *)(normals_buffer.data() + normals_offset);

        part.normals.resize(normals_length / sizeof(float));
        std::memcpy(part.normals.data(), normals_start, normals_length);
    }

    {
        auto uvs_accessor_index = primitive["attributes"]["TEXCOORD_0"].get<uint32_t>();
        auto uvs_view_index = accessors[uvs_accessor_index]["bufferView"].get<uint32_t>();
        auto &uvs_view = gltf_json["bufferViews"][uvs_view_index];

        auto uvs_buffer_index = uvs_view["buffer"].get<uint32_t>();
        auto uvs_offset = uvs_view["byteOffset"].get<size_t>();
        auto uvs_length = uvs_view["byteLength"].get<size_t>();

        auto &uvs_buffer = buffers_content[uvs_buffer_index];
        auto *uvs_start = (float *)(uvs_buffer.data() + uvs_offset);

        part.uvs.resize(uvs_length / sizeof(float));
        std::memcpy(part.uvs.data(), uvs_start, uvs_length);
    }

    part.mat = materials[primitive["material"].get<uint32_t>()];
    part.vertices_count = part.positions.size() / 3;
    part.triangles_count = part.indices.size() / 3;

    return part;
}
