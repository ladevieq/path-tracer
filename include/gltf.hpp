#ifndef __GLTF_HPP_
#define __GLTF_HPP_

#include <vector>

#include "utils.hpp"
#include "handle.hpp"
#include "mesh.hpp"
#include "material.hpp"

struct node {
    std::vector<uint32_t>    children;
    u32                 mesh;
};

struct raw_image {
    int32_t width;
    int32_t height;
    int32_t channels;
    const uint8_t *data;
};

struct buffer {
    u8* bytes;
    size_t size;
};

class gltf {
    public:
    gltf(const char* filepath, bool is_glb = false);
    void deinit();

    std::vector<u32> root_nodes;

    std::vector<node> nodes;
    std::vector<Mesh> meshes;
    std::vector<material> materials;
    std::vector<handle<device_texture>> textures;
    // handle<device_sampler> samplers;
    std::vector<struct buffer> buffers;


    // gbl only
    struct glb_chunk {
        u32 length;
        u32 type;
        u8 data[1];
    };

    struct glb_header {
        u32 magic;
        u32 version;
        u32 length;
    };
    std::vector<struct glb_chunk*> chunks;
    glb_header* header = nullptr;

    // TODO: remove
    bool is_glb;
};

#endif
