#ifndef __GLTF_HPP_
#define __GLTF_HPP_

#include <filesystem>
#include <vector>
#include <optional>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

#include <vec3.hpp>

struct mesh_part {
    std::vector<uint16_t> indices;
    std::vector<vec3> positions;
    // std::vector<vec3> normals;
};

struct mesh {
    std::vector<mesh_part> parts;
};

struct node {
    std::vector<node> children;
    std::optional<mesh> mesh;
};

class gltf {
public:
    static node load(std::filesystem::path &path, std::string &filename);

private:

    static void load_node(json &gltf_json, uint32_t index, node &parent, std::vector<mesh> &meshes);

    static std::vector<mesh> load_meshes(json &gltf_json, std::vector<std::vector<uint8_t>> &buffers_content);

    static mesh_part load_primitive(json &gltf_json, json &primitive, std::vector<std::vector<uint8_t>> &buffers_content);
};

#endif
