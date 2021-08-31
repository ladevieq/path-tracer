#include "gltf.hpp"

#include <fstream>
#include <cstring>

#include "utils.hpp"

const vec3 scale (0.00800000037997961);

node gltf::load(std::filesystem::path &path, std::string &filename) {
    std::fstream f(path / filename);
    json gltf_json;

    f >> gltf_json;

    auto &scene = gltf_json["scenes"][0];

    auto &buffers = gltf_json["buffers"];
    auto buffer_index = 0;
    auto buffers_content = std::vector<std::vector<uint8_t>>(buffers.size());
    for (auto &buffer: buffers) {
        auto buffer_path = (path / buffer["uri"].get<std::string>()).string();
        buffers_content[buffer_index] = read_file(buffer_path);
        buffer_index++;
    }

    // auto materials = load_materials(gltf_json);
    std::vector<material> materials;

    auto meshes = load_meshes(gltf_json, buffers_content, materials);

    node root_node;
    auto &root_children = scene["nodes"];
    uint32_t root_children_count = scene["nodes"].size();
    root_node.children.resize(root_children_count);

    for (size_t child_index = 0; child_index < root_children_count; child_index++) {
        load_node(gltf_json, root_children[child_index].get<uint32_t>(), root_node.children[child_index], meshes);
    }

    return std::move(root_node);
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

    if (gltf_node.contains("mesh")) {
        parent.mesh = std::make_optional(meshes[gltf_node["mesh"].get<uint32_t>()]);
    }
}

std::vector<mesh> gltf::load_meshes(json &gltf_json, std::vector<std::vector<uint8_t>> &buffers_content, std::vector<material> &materials) {
    auto &gltf_meshes = gltf_json["meshes"];
    auto meshes_count = gltf_meshes.size();
    std::vector<mesh> meshes { meshes_count };

    for(size_t mesh_index = 0; mesh_index < meshes_count; mesh_index++) {
        auto &gltf_mesh = gltf_meshes[mesh_index];
        auto &primitives = gltf_mesh["primitives"];
        auto &mesh = meshes[mesh_index];

        for (auto& primitive: primitives) {
            mesh.parts.emplace_back(load_primitive(gltf_json, primitive, buffers_content, materials));
        }
    }

    return std::move(meshes);
}

std::vector<material> gltf::load_materials(json &gltf_json) {
    auto &gltf_materials = gltf_json["materials"];
    auto materials_count = gltf_materials.size();
    std::vector<material> materials { materials_count };

    for(size_t material_index = 0; material_index < materials_count; material_index++) {
        auto& gltf_material = gltf_materials[material_index];
        auto& material = materials[material_index];

        auto base_color = gltf_material["baseColorFactor"];
        material.base_color.v[0] = base_color[0];
        material.base_color.v[1] = base_color[1];
        material.base_color.v[2] = base_color[2];
        material.base_color.v[3] = base_color[3];
        material.metalness = gltf_material["matellicFactor"].get<float>();
        material.roughness = gltf_material["roughnessFactor"].get<float>();
    }

    return std::move(materials);
}

mesh_part gltf::load_primitive(json &gltf_json, json &primitive, std::vector<std::vector<uint8_t>> &buffers_content, std::vector<material> &materials) {
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
        uint16_t* indices_start = (uint16_t*)(indices_buffer.data() + indices_offset);

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
        float* positions_start = (float*)(positions_buffer.data() + positions_offset);

        size_t positions_count = positions_length / (sizeof(float) * 3);
        part.positions.resize(positions_count);

        for (size_t i = 0; i < positions_count; i++) {
            part.positions[i] = vec3(positions_start[i * 3], positions_start[i * 3 + 1], positions_start[i * 3 + 2]) * scale;
        }
    }

    {
        auto normals_accessor_index = primitive["attributes"]["NORMAL"].get<uint32_t>();
        auto normals_view_index = accessors[normals_accessor_index]["bufferView"].get<uint32_t>();
        auto &normals_view = gltf_json["bufferViews"][normals_view_index];

        auto normals_buffer_index = normals_view["buffer"].get<uint32_t>();
        auto normals_offset = normals_view["byteOffset"].get<size_t>();
        auto normals_length = normals_view["byteLength"].get<size_t>();

        auto &normals_buffer = buffers_content[normals_buffer_index];
        float* normals_start = (float*)(normals_buffer.data() + normals_offset);

        size_t normals_count = normals_length / (sizeof(float) * 3);
        part.normals.resize(normals_count);

        for (size_t i = 0; i < normals_count; i++) {
            part.normals[i] = vec3(normals_start[i * 3], normals_start[i * 3 + 1], normals_start[i * 3 + 2]);
        }
    }

    // part.mat = materials[primitive["material"].get<uint32_t>()];

    return std::move(part);
}
