#ifndef __MATERIAL_HPP_
#define __MATERIAL_HPP_

#include <cstdint>

#include "vec3.hpp"

struct texture {
    uint32_t image_id;
    uint32_t sampler_id;
    uint32_t padding[2];
};

struct material {
    color base_color;
    texture base_color_texture;
    texture metallic_roughness_texture;
    float metalness = 1.f;
    float roughness = 1.f;
};

#endif // !__MATERIAL_HPP_
