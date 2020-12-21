#ifndef __SPHERE_HPP_
#define __SPHERE_HPP_

#include "hittable.hpp"
#include "material.hpp"

class sphere : public hittable {
    public:
        sphere(point3 center, double radius, std::shared_ptr<material> material);

        virtual bool hit(const ray& r, double t_min, double t_max, struct hit_info& info) const;

        point3 center;
        double radius;

        std::shared_ptr<material> mat_ptr;
};

#endif // !__SPHERE_HPP_
