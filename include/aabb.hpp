#ifndef __AABB_HPP_
#define __AABB_HPP_

#include "vec3.hpp"
#include "triangle.hpp"
#include <cmath>
#include <limits.h>

class aabb {
    public:
        aabb() = default;
        aabb(vec3 &min_point, vec3 &max_point): minimum(min_point), maximum(max_point) {}
        aabb(vec3 &position, float radius): minimum(position - vec3(radius)), maximum(position + vec3(radius)) {}
        aabb(triangle &tri) {
            auto &v1 = tri.v1.position;
            auto &v2 = tri.v2.position;
            auto &v3 = tri.v3.position;

            // minimum.x = fmin(fmin(v1.x, v2.x), v3.x);
            // minimum.y = fmin(fmin(v1.y, v2.y), v3.y);
            // minimum.z = fmin(fmin(v1.z, v2.z), v3.z);

            minimum.v[0] = fmin(fmin(v1.v[0], v2.v[0]), v3.v[0]);
            minimum.v[1] = fmin(fmin(v1.v[1], v2.v[1]), v3.v[1]);
            minimum.v[2] = fmin(fmin(v1.v[2], v2.v[2]), v3.v[2]);

            // maximum.x = fmax(fmax(v1.x, v2.x), v3.x);
            // maximum.y = fmax(fmax(v1.y, v2.y), v3.y);
            // maximum.z = fmax(fmax(v1.z, v2.z), v3.z);

            maximum.v[0] = fmax(fmax(v1.v[0], v2.v[0]), v3.v[0]);
            maximum.v[1] = fmax(fmax(v1.v[1], v2.v[1]), v3.v[1]);
            maximum.v[2] = fmax(fmax(v1.v[2], v2.v[2]), v3.v[2]);
        }

        void union_with(const aabb &box) {
            minimum.min(box.minimum);
            maximum.max(box.maximum);
        }

        void union_with(const point3 &point) {
            minimum.min(point);
            maximum.max(point);
        }

        int32_t maximum_axis() {
            vec3 diag = diagonal();

            // if (diag.x > diag.y && diag.x > diag.z) {
            //     return 0;
            // } else if (diag.y > diag.z) {
            //     return 1;
            // } else {
            //     return 2;
            // }

            if (diag.v[0] > diag.v[1] && diag.v[0] > diag.v[2]) {
                return 0;
            } else if (diag.v[1] > diag.v[2]) {
                return 1;
            } else {
                return 2;
            }
        }

        vec3 diagonal() {
            return maximum - minimum;
        }

        vec3 center() {
            return diagonal() / 2.f;
        }

        float surface_area() {
            auto diag = diagonal();
            // return 2.f * (diag.x * diag.y + diag.x * diag.z + diag.y * diag.z);
            return 2.f * (diag.v[0] * diag.v[1] + diag.v[0] * diag.v[2] + diag.v[1] * diag.v[2]);
        }

        vec3 minimum = vec3(std::numeric_limits<float>::max());
        vec3 maximum = vec3(std::numeric_limits<float>::lowest());
};

#endif // !__AABB_HPP_
