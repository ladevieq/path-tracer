#pragma once

#include <cstdint>

#include "vec3.hpp"

class Texture;

struct gpu_texture {
    uint32_t image_id;
    uint32_t sampler_id;
    uint32_t padding[2];
};

struct material {
    color base_color;
    Texture* base_color_texture;
    Texture* metallic_roughness_texture;
    float metalness = 1.f;
    float roughness = 1.f;
};
