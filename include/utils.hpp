#ifndef __UTILS_HPP_
#define __UTILS_HPP_

#include <cstdint>
#include <cstdlib>
#include <vector>

#include "vec3.hpp"
#include "sphere.hpp"
#include "camera.hpp"

struct input_data {
    color sky_color;
    color ground_color;

    camera cam;

    float random_offset[2000];
    float random_disk[2000];
    vec3 random_in_sphere[1000];

    sphere spheres[512];

    uint32_t max_bounce;
    uint32_t samples_per_pixel;

    // Output image resolution
    uint32_t width;
    uint32_t height;

    uint32_t sample_index;
};

std::vector<uint8_t> get_shader_code(const char* path);

struct input_data create_inputs(uint32_t width, uint32_t height, uint32_t samples_per_pixel);

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
inline float clamp(float x, float min, float max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

#endif // !__UTILS_HPP_
