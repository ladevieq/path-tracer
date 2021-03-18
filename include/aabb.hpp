#ifndef __AABB_HPP_
#define __AABB_HPP_

#include "vec3.hpp"

class AABB {
    AABB(vec3 min_point, vec3 max_point): minimum(min_point), maximum(max_point) {}

    vec3 minimum;
    vec3 maximum;
};

#endif // !__AABB_HPP_
