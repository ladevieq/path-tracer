#ifndef __SCENE_HPP_
#define __SCENE_HPP_

#include <cstdint>

#include "camera.hpp"

class Buffer;
class scene {
    struct metadata {
        camera cam;

        uint32_t max_bounce = 3;
        uint32_t min_bounce = 1;

        uint32_t width;
        uint32_t height;

        uint32_t sample_index = 0;

        uint32_t enable_dof = (uint32_t)false;
        uint32_t debug_bvh  = (uint32_t)false;
        int32_t downscale_factor = 1;

        metadata(const camera &cam, uint32_t width, uint32_t height);
    };

    // void random_scene();

public:
    scene(const camera& cam, uint32_t width, uint32_t height);

    metadata meta;

    Buffer*                 scene_buffer;
    Buffer*                 indices_buffer;
    Buffer*                 positions_buffer;
    Buffer*                 normals_buffer;
    Buffer*                 uvs_buffer;
    Buffer*                 bvh_buffer;
    Buffer*                 materials_buffer;
};

#endif // !__SCENE_HPP_
