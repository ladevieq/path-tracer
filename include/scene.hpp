#ifndef __SCENE_HPP_
#define __SCENE_HPP_

class Buffer;

enum FEATURES_BIT_INDEX: u8 {
    DOF,
    BVH_DEBUG_VIEW,
};

struct metadata {
    camera cam;

    u8 max_bounce = 3;
    u8 min_bounce = 1;

    u8 features = 0b0; // 0: dof, 1: bvh debug view
    u8 downscale_factor = 1;

    u16 width;
    u16 height;

    u32 sample_index = 1;


    metadata(const camera &cam, u16 width, u16 height);
};

class scene {
    // void random_scene();
    gltf model;

public:
    scene(const camera& cam, u16 width, u16 height);
    void deinit();

    metadata meta;

    handle<device_buffer>    indices_buffer_handle;
    handle<device_buffer>    positions_buffer_handle;
    handle<device_buffer>    normals_buffer_handle;
    handle<device_buffer>    uvs_buffer_handle;
    handle<device_buffer>    bvh_buffer_handle;
    handle<device_buffer>    materials_buffer_handle;

    uint64_t indices_buffer_address() { return vkdevice::get().get_buffer(indices_buffer_handle)->device_address; };
    uint64_t positions_buffer_address() { return vkdevice::get().get_buffer(positions_buffer_handle)->device_address; };
    uint64_t normals_buffer_address() { return vkdevice::get().get_buffer(normals_buffer_handle)->device_address; };
    uint64_t uvs_buffer_address() { return vkdevice::get().get_buffer(uvs_buffer_handle)->device_address; };
    uint64_t bvh_buffer_address() { return vkdevice::get().get_buffer(bvh_buffer_handle)->device_address; };
    uint64_t materials_buffer_address() { return vkdevice::get().get_buffer(materials_buffer_handle)->device_address; };
};

#endif // !__SCENE_HPP_
