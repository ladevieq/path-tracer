#ifndef __RT_HPP_
#define __RT_HPP_

#include <limits>

const double pi = 3.1415926535897932385;
const double infinity = std::numeric_limits<double>::infinity();

inline double deg_to_rad(double deg) {
    return deg * pi / 180.0;
}

#include "vec3.hpp"
#include "ray.hpp"
#include "color.hpp"
#include "sphere.hpp"

#endif // !__RT_HPP_
