#ifndef __GLTF_HPP_
#define __GLTF_HPP_

#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

#include "material.hpp"
#include "mesh.hpp"

struct node {
    std::vector<node>   children;
    Mesh*               mesh = nullptr;
};

struct raw_image {
    int32_t width;
    int32_t height;
    int32_t channels;
    uint8_t *data;
};

namespace std::filesystem {
    class path;
}

class gltf {
    public:
    gltf(const std::filesystem::path &filepath);

    node root_node;

private:

    void load_node(uint32_t index, node &parent);

    void load_meshes();

    Mesh::submesh load_primitive(const json &primitive);

    Mesh::attribute load_attribute(uint32_t accessor_index);

    void load_textures(const std::filesystem::path &path);

    void load_materials();

    json gltf_json;
    std::vector<Mesh> meshes;
    std::vector<material> materials;
    std::vector<Texture*> textures;
    std::vector<std::vector<uint8_t>> buffers;
};

#endif
