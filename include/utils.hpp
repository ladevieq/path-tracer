#ifndef __UTILS_HPP_
#define __UTILS_HPP_

#include <cstdint>
#include <cstdlib>
#include <vector>

#include "vec3.hpp"
#include "sphere.hpp"
#include "camera.hpp"
#include "bvh-node.hpp"

struct input_data {
    color sky_color;
    color ground_color;

    camera cam;

    sphere spheres[512];
    bvh_node nodes[1024];

    uint32_t max_bounce;

    // Output image resolution
    uint32_t width;
    uint32_t height;

    uint32_t sample_index;

    uint32_t spheres_count;
    uint32_t debug_bvh;
    uint32_t padding[2];
};

std::vector<uint8_t> get_shader_code(const char* path);
std::vector<uint8_t> get_shader_code(std::string& path);

struct input_data create_inputs(uint32_t width, uint32_t height);

#define PI 3.14159265359

inline float deg_to_rad(float deg) {
    return deg * PI / 180.0;
}

inline float randd() {
    return rand() / (RAND_MAX + 1.0);
}

inline float randd(float min, float max) {
    return min + randd() * (max - min);
}

inline int32_t randi(int32_t min, int32_t max) {
    return static_cast<int32_t>(randd(min, max + 1));
}

inline float clamp(float x, float min, float max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

#endif // !__UTILS_HPP_
