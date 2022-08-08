#pragma once

#include <cstdint>
#include <vector>
#include <array>

#include "vk-renderer.hpp"

enum ATTRIBUTE_TYPE {
    POSITION,
    NORMAL,
    TANGENT,
    UV_0,
    MAX_ATTRIBUTE,
};

struct material;

class Mesh {
    struct submesh_info {
        uint32_t    vertex_offset;
        uint32_t    index_offset;
        size_t      element_count;
        uint32_t    mat_id;
    };

public:
    struct mesh_instance {
        const Mesh& mesh;
        // transform a transform matrice for the instance
    };

    struct attribute {
        uint8_t* data = nullptr;
        size_t length = 0;
    };

    struct submesh {
        attribute indices;
        std::array<attribute, ATTRIBUTE_TYPE::MAX_ATTRIBUTE> attributes;
        uint32_t vertex_count = 0;
        uint32_t index_count = 0;
        // material& mat;
    };

    Mesh() = default;

    [[nodiscard]] mesh_instance instantiate () const;

    void add_submesh(const submesh& submesh);

    static inline uint32_t attribute_components(enum ATTRIBUTE_TYPE attribute_type) {
        switch(attribute_type) {
            case ATTRIBUTE_TYPE::POSITION:
            case ATTRIBUTE_TYPE::NORMAL:
                return 3;
            case ATTRIBUTE_TYPE::UV_0:
                return 2;
            default:
                return 0;
        }
    }

    static inline size_t attribute_size(enum ATTRIBUTE_TYPE attribute_type) {
        return sizeof(float) * attribute_components(attribute_type);
    }

    [[nodiscard]] inline uint32_t triangle_count() const { return index_count / 3; };
    [[nodiscard]] inline uint32_t vertices_count() const { return vertex_count; };


    static void register_mesh(const Mesh& mesh);

private:

    uint32_t vertex_count;
    uint32_t index_count;

    std::array<std::vector<float>, ATTRIBUTE_TYPE::MAX_ATTRIBUTE> attributes;
    std::vector<uint16_t> indices;

    std::vector<submesh_info> submeshes;

    struct gpu_mesh {
        std::array<Buffer*, ATTRIBUTE_TYPE::MAX_ATTRIBUTE> attributes;
        Buffer* indices;
        Buffer* submeshes;
    };

    // allocate gpu mesh upon instanciation
    // static std::vector<gpu_mesh> gpu_meshes;
    // static std::vector<transform> instances;
};
