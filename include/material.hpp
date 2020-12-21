#ifndef __MATERIAL_HPP_
#define __MATERIAL_HPP_

#include "ray.hpp"

struct hit_info;

class material {
    public:
        virtual bool scatter(const ray& r, const struct hit_info& info, color& attenuation, ray& scaterred) const = 0;
};

#endif // !__MATERIAL_HPP_
