#ifndef __GLTF_HPP_
#define __GLTF_HPP_

#include <filesystem>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

#include <vec3.hpp>

struct mesh_part {
    std::vector<uint16_t> indices;
    std::vector<vec3> positions;
    std::vector<vec3> normals;
};

struct mesh {
    std::vector<std::vector<mesh_part>> nodes;
};

class gltf {
public:
    static mesh load(std::filesystem::path &path, std::string &filename);

private:

    static void load_node(json &gltf_json, json &node, std::vector<std::vector<uint8_t>> &buffers_content, mesh& gltf_mesh);

    static void load_primitive(json &gltf_json, json &primitive, std::vector<std::vector<uint8_t>> &buffers_content, mesh& gltf_mesh);
};

#endif
