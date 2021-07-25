#include "gltf.hpp"

#include <fstream>
#include <cstring>

#include "utils.hpp"

mesh gltf::load(std::filesystem::path &path, std::string &filename) {
    std::fstream f(path / filename);
    json gltf_json;

    f >> gltf_json;

    auto scene_index = gltf_json["scene"].get<uint32_t>();
    auto scenes = gltf_json["scenes"];
    auto scene = scenes[scene_index];

    auto buffers = gltf_json["buffers"];
    auto buffer_index = 0;
    auto buffers_content = std::vector<std::vector<uint8_t>>(buffers.size());
    for (auto &buffer: buffers) {
        auto buffer_path = (path / buffer["uri"].get<std::string>()).string();
        buffers_content[buffer_index] = read_file(buffer_path);
        buffer_index++;
    }

    auto nodes = gltf_json["nodes"];
    auto gltf_mesh = mesh();

    for (auto &node: nodes) {
        gltf_mesh.nodes.push_back(std::vector<mesh_part>());
        load_node(gltf_json, node, buffers_content, gltf_mesh);
    }

    // return std::move(gltf_mesh);
    return gltf_mesh;
}

void gltf::load_node(json &gltf_json, json &node, std::vector<std::vector<uint8_t>> &buffers_content, mesh& gltf_mesh) {
    auto mesh_index = node["mesh"].get<uint32_t>();
    auto mesh = gltf_json["meshes"][mesh_index];
    auto primitives = mesh["primitives"];

    for (auto &primitive: primitives) {
        gltf_mesh.nodes.back().push_back(mesh_part());
        load_primitive(gltf_json, primitive, buffers_content, gltf_mesh);
    }
}

void gltf::load_primitive(json &gltf_json, json &primitive, std::vector<std::vector<uint8_t>> &buffers_content, mesh& gltf_mesh) {
    auto accessors = gltf_json["accessors"];
    auto &mesh_part = gltf_mesh.nodes.back().back();

    {
        auto indices_accessor_index = primitive["indices"].get<uint32_t>();
        auto indices_view_index = accessors[indices_accessor_index]["bufferView"].get<uint32_t>();
        auto indices_view = gltf_json["bufferViews"][indices_view_index];

        auto indices_buffer_index = indices_view["buffer"].get<uint32_t>();
        auto indices_offset = indices_view["byteOffset"].get<size_t>();
        auto indices_length = indices_view["byteLength"].get<size_t>();

        auto indices_buffer = buffers_content[indices_buffer_index];
        void* indices_start = indices_buffer.data() + indices_offset;

        mesh_part.indices.resize(indices_length / sizeof(uint16_t));

        std::memcpy(mesh_part.indices.data(), indices_start, indices_length);
    }

    {
        auto positions_accessor_index = primitive["attributes"]["POSITION"].get<uint32_t>();
        auto positions_view_index = accessors[positions_accessor_index]["bufferView"].get<uint32_t>();
        auto positions_view = gltf_json["bufferViews"][positions_view_index];

        auto positions_buffer_index = positions_view["buffer"].get<uint32_t>();
        auto positions_offset = positions_view["byteOffset"].get<size_t>();
        auto positions_length = positions_view["byteLength"].get<size_t>();

        auto positions_buffer = buffers_content[positions_buffer_index];
        float* positions_start = (float*)(positions_buffer.data() + positions_offset);

        mesh_part.positions.resize(positions_length / (sizeof(float) * 3));

        for (size_t i = 0; i < positions_length / (sizeof(float) * 3); i++) {
            mesh_part.positions[i] = vec3(positions_start[i * 3], positions_start[i * 3 + 1], positions_start[i * 3 + 2]);
        }
        // std::memcpy(mesh_part.positions.data(), positions_start, positions_length);
    }

    {
        auto normals_accessor_index = primitive["attributes"]["NORMAL"].get<uint32_t>();
        auto normals_view_index = accessors[normals_accessor_index]["bufferView"].get<uint32_t>();
        auto normals_view = gltf_json["bufferViews"][normals_view_index];

        auto normals_buffer_index = normals_view["buffer"].get<uint32_t>();
        auto normals_offset = normals_view["byteOffset"].get<size_t>();
        auto normals_length = normals_view["byteLength"].get<size_t>();

        auto normals_buffer = buffers_content[normals_buffer_index];
        void* normals_start = normals_buffer.data() + normals_offset;

        mesh_part.normals.resize(normals_length / (sizeof(float) * 3));

        std::memcpy(mesh_part.normals.data(), normals_start, normals_length);
    }
}
