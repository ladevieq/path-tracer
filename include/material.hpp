#pragma once

#include <cstdint>

#include "vec3.hpp"

#ifdef API_TEST
#include "vk-device-types.hpp"
#else
class Texture;
#endif

struct material {
    color base_color;
#ifdef API_TEST
    handle<device_texture> base_color_texture;
    handle<device_texture> metallic_roughness_texture;
#else
    Texture* base_color_texture;
    Texture* metallic_roughness_texture;
#endif // API_TEST
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
