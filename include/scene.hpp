#ifndef __SCENE_HPP_
#define __SCENE_HPP_

#include "vec3.hpp"
#include "sphere.hpp"
#include "camera.hpp"
#include "bvh-node.hpp"


class scene {
public:
    scene(uint32_t width, uint32_t height);

    color sky_color = { 0.5, 0.7, 1.0 };
    color ground_color = { 1.0, 1.0, 1.0 };

    camera cam;

    sphere spheres[512] = {};
    bvh_node nodes[1024] = {};

    uint32_t max_bounce = 50;

    // Output image resolution
    uint32_t width;
    uint32_t height;

    uint32_t sample_index = 0;

    uint32_t spheres_count = 0;
    uint32_t enable_dof = true;
    uint32_t debug_bvh = false;
    uint32_t padding;

private:

    uint32_t random_scene(sphere* world);

};

#endif // !__SCENE_HPP_
