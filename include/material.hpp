#pragma once

struct material {
    color base_color;
    handle<device_texture> base_color_texture;
    handle<device_texture> metallic_roughness_texture;
    float metalness = 1.f;
    float roughness = 1.f;
};

struct gpu_material {
    color base_color;
    uint32_t albedo_texture_id = 0;
    uint32_t albedo_texture_sampler_id = 0;
    uint32_t metallic_roughness_texture_id = 0;
    uint32_t metallic_roughness_texture_sampler_id = 0;
    float metalness = 1.f;
    float roughness = 1.f;
};
