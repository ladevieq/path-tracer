#ifndef __AABB_HPP_
#define __AABB_HPP_

#include "vec3.hpp"

class aabb {
    public:
        aabb() = default;
        aabb(vec3 min_point, vec3 max_point): minimum(min_point), maximum(max_point) {}
        aabb(vec3 position, float radius): minimum(position - vec3(radius)), maximum(position + vec3(radius)) {}

        static aabb surrounding_box(aabb box0, aabb box1) {
            point3 min_point(fmin(box0.minimum.x, box1.minimum.x),
                         fmin(box0.minimum.y, box1.minimum.y),
                         fmin(box0.minimum.z, box1.minimum.z));

            point3 max_point(fmax(box0.maximum.x, box1.maximum.x),
                       fmax(box0.maximum.y, box1.maximum.y),
                       fmax(box0.maximum.z, box1.maximum.z));

            return aabb(min_point, max_point);
        }

        vec3 minimum;
        vec3 maximum;
};

#endif // !__AABB_HPP_
