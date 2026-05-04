#ifndef __SCENE_HPP_
#define __SCENE_HPP_

class Buffer;

struct metadata {
    camera cam;

    uint32_t max_bounce = 3;
    uint32_t min_bounce = 1;

    uint32_t width;
    uint32_t height;

    uint32_t sample_index = 1;

    uint32_t enable_dof = (uint32_t)false;
    uint32_t debug_bvh  = (uint32_t)false;
    int32_t downscale_factor = 1;

    metadata(const camera &cam, uint32_t width, uint32_t height);
};

class scene {
    // void random_scene();
    gltf model;

public:
    scene(const camera& cam, uint32_t width, uint32_t height);
    void deinit();

    metadata meta;

    handle<device_buffer>    indices_buffer_handle;
    handle<device_buffer>    positions_buffer_handle;
    handle<device_buffer>    normals_buffer_handle;
    handle<device_buffer>    uvs_buffer_handle;
    handle<device_buffer>    bvh_buffer_handle;
    handle<device_buffer>    materials_buffer_handle;

    uint64_t indices_buffer_address() { return vkdevice::get().get_buffer(indices_buffer_handle).device_address; };
    uint64_t positions_buffer_address() { return vkdevice::get().get_buffer(positions_buffer_handle).device_address; };
    uint64_t normals_buffer_address() { return vkdevice::get().get_buffer(normals_buffer_handle).device_address; };
    uint64_t uvs_buffer_address() { return vkdevice::get().get_buffer(uvs_buffer_handle).device_address; };
    uint64_t bvh_buffer_address() { return vkdevice::get().get_buffer(bvh_buffer_handle).device_address; };
    uint64_t materials_buffer_address() { return vkdevice::get().get_buffer(materials_buffer_handle).device_address; };
};

#endif // !__SCENE_HPP_
