#ifndef __SCENE_HPP_
#define __SCENE_HPP_

#include "vec3.hpp"
#include "sphere.hpp"
#include "triangle.hpp"
#include "camera.hpp"
#include "bvh-node.hpp"

#include "vk-renderer.hpp"

class scene {
    struct metadata {
        camera cam;

        uint32_t max_bounce = 3;
        uint32_t min_bounce = 1;

        uint32_t width;
        uint32_t height;

        uint32_t sample_index = 0;

        uint32_t enable_dof = false;
        uint32_t debug_bvh  = false;
        int32_t downscale_factor = 1;

        metadata(camera cam, uint32_t width, uint32_t height)
            : cam(cam), width(width), height(height) {}
    };

    // void random_scene();

public:
    scene(camera cam, uint32_t width, uint32_t height, vkrenderer &renderer);

    void* scene_buffer_ptr() {
        return scene_buffer.alloc_info.pMappedData;
    }

    metadata meta;

    // Geometry & BVH
    std::vector<uint32_t>   indices;
    std::vector<float>      positions;
    std::vector<float>      normals;
    std::vector<float>      uvs;
    std::vector<material>   materials;

    buffer                  scene_buffer;
    buffer                  indices_buffer;
    buffer                  positions_buffer;
    buffer                  normals_buffer;
    buffer                  uvs_buffer;
    buffer                  bvh_buffer;
    buffer                  materials_buffer;

    std::vector<packed_bvh_node> packed_nodes;
};

#endif // !__SCENE_HPP_
