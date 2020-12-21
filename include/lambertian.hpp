#ifndef __LAMBERTIAN_HPP_
#define __LAMBERTIAN_HPP_

#include "material.hpp"

class lambertian : public material {
    public:
        lambertian(const color& albedo);

        virtual bool scatter(const ray& r, const struct hit_info& info, color& attenuation, ray& scattered) const;

        color albedo;
};

#endif // !__LAMBERTIAN_HPP_
