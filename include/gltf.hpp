#ifndef __GLTF_HPP_
#define __GLTF_HPP_

#include <filesystem>
#include <vector>
#include <optional>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

#include "material.hpp"
#include "vec3.hpp"

#include "vk-renderer.hpp"

struct mesh_part {
    std::vector<uint16_t>   indices;
    std::vector<float>      positions;
    std::vector<float>      normals;
    std::vector<float>      uvs;
    material                mat;
    uint32_t triangles_count = 0;
    uint32_t vertices_count = 0;
};

struct mesh {
    std::vector<mesh_part> parts;
    uint32_t triangles_count = 0;
    uint32_t vertices_count = 0;
};

struct node {
    std::vector<node> children;
    std::optional<mesh> mesh;
};

class gltf {
public:
    static node load(std::filesystem::path &path, std::string &filename, vkrenderer &renderer);

private:

    static void load_node(json &gltf_json, uint32_t index, node &parent, std::vector<mesh> &meshes);

    static std::vector<mesh> load_meshes(json &gltf_json, std::vector<std::vector<uint8_t>> &buffers_content, std::vector<material> &materials);

    static std::vector<texture> load_textures(json &gltf_json, std::filesystem::path &path, vkrenderer &renderer);

    static std::vector<material> load_materials(json &gltf_json, std::vector<texture> &textures);

    static mesh_part load_primitive(json &gltf_json, json &primitive, std::vector<std::vector<uint8_t>> &buffers_content, std::vector<material> &materials);
};

#endif
