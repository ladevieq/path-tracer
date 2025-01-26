#ifndef __SCENE_HPP_
#define __SCENE_HPP_

#include <cstdint>

#ifdef API_TEST
#include "vk-device-types.hpp"
#include "vk-device.hpp"
#endif // API_TEST
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

#ifdef API_TEST
    handle<device_buffer>    scene_buffer_handle;
    handle<device_buffer>    indices_buffer_handle;
    handle<device_buffer>    positions_buffer_handle;
    handle<device_buffer>    normals_buffer_handle;
    handle<device_buffer>    uvs_buffer_handle;
    handle<device_buffer>    bvh_buffer_handle;
    handle<device_buffer>    materials_buffer_handle;

    uint64_t scene_buffer_address() { return vkdevice::get_render_device().get_buffer(scene_buffer_handle).device_address; };
    uint64_t indices_buffer_address() { return vkdevice::get_render_device().get_buffer(indices_buffer_handle).device_address; };
    uint64_t positions_buffer_address() { return vkdevice::get_render_device().get_buffer(positions_buffer_handle).device_address; };
    uint64_t normals_buffer_address() { return vkdevice::get_render_device().get_buffer(normals_buffer_handle).device_address; };
    uint64_t uvs_buffer_address() { return vkdevice::get_render_device().get_buffer(uvs_buffer_handle).device_address; };
    uint64_t bvh_buffer_address() { return vkdevice::get_render_device().get_buffer(bvh_buffer_handle).device_address; };
    uint64_t materials_buffer_address() { return vkdevice::get_render_device().get_buffer(materials_buffer_handle).device_address; };
#else
    Buffer*                 scene_buffer;
    Buffer*                 indices_buffer;
    Buffer*                 positions_buffer;
    Buffer*                 normals_buffer;
    Buffer*                 uvs_buffer;
    Buffer*                 bvh_buffer;
    Buffer*                 materials_buffer;
#endif // API_TEST
};

#endif // !__SCENE_HPP_
