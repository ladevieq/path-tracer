#ifndef __SPHERE_HPP_
#define __SPHERE_HPP_

#include "hittable.hpp"

class sphere : public hittable {
    public:
        sphere(point3 center, double radius = 1.0);

        virtual bool hit(const ray& r, double t_min, double t_max, struct hit_info& info) const;

        point3 center;
        double radius;
};

#endif // !__SPHERE_HPP_
