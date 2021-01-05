#ifndef __RT_HPP_
#define __RT_HPP_

#include <limits>
#include <cstdlib>

const float pi = 3.1415926535897932385f;
const float infinity = std::numeric_limits<float>::infinity();

inline float deg_to_rad(float deg) {
    return deg * pi / 180.0;
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


#include "vec3.hpp"
#include "ray.hpp"
#include "color.hpp"
// #include "sphere.hpp"
#include "camera.hpp"
// #include "material.hpp"
// #include "lambertian.hpp"
// #include "metal.hpp"
// #include "dielectric.hpp"
// #include "hittable_list.hpp"

#endif // !__RT_HPP_
