#ifndef __RT_HPP_
#define __RT_HPP_

#include <limits>
#include <cstdlib>

const double pi = 3.1415926535897932385;
const double infinity = std::numeric_limits<double>::infinity();

inline double deg_to_rad(double deg) {
    return deg * pi / 180.0;
}

inline double randd() {
    return rand() / (RAND_MAX + 1.0);
}

inline double randd(double min, double max) {
    return min + randd() * (max - min);
}

inline double clamp(double x, double min, double max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}


#include "vec3.hpp"
#include "ray.hpp"
#include "color.hpp"
#include "sphere.hpp"
#include "camera.hpp"
#include "material.hpp"
#include "lambertian.hpp"
#include "metal.hpp"
#include "dielectric.hpp"
#include "hittable_list.hpp"

#endif // !__RT_HPP_
