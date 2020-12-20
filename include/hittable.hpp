#ifndef __HITTABLE_HPP_
#define __HITTABLE_HPP_

#include "ray.hpp"

struct hit_info {
    double t;
    point3 point;
    vec3 normal;
    bool front_face;

    inline void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = r.direction.dot(outward_normal) < 0.0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable {
    public:
        virtual bool hit(const ray& r, double t_min, double t_max, struct hit_info& info) const = 0;
};

#endif // !__HITTABLE_HPP_
