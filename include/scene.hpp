#ifndef __SCENE_HPP_
#define __SCENE_HPP_

#include "vec3.hpp"
#include "sphere.hpp"
#include "triangle.hpp"
#include "camera.hpp"
#include "bvh-node.hpp"


class scene {
    struct metadata {
        camera cam;

        // uint32_t max_bounce = 50;
        uint32_t max_bounce = 4;

        // Output image resolution
        uint32_t width;
        uint32_t height;

        uint32_t sample_index = 0;

        // uint32_t spheres_count = 0;
        uint32_t triangles_count = 0;
        uint32_t enable_dof = false;
        uint32_t debug_bvh = false;
        uint32_t padding;

        metadata(camera cam, uint32_t width, uint32_t height)
            : cam(cam), width(width), height(height) {}
    };

    void random_scene();

public:
    scene(camera cam, uint32_t width, uint32_t height);

    metadata meta;

    // Geometry & BVH
    // std::vector<sphere> spheres;
    std::vector<triangle> triangles;
    std::vector<packed_bvh_node> packed_nodes;
};

#endif // !__SCENE_HPP_
