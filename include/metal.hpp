#ifndef __METAL_HPP_
#define __METAL_HPP_

#include "material.hpp"

class metal : public material {
    public:
        metal(const color& albedo, double f);

        virtual bool scatter(const ray& r, const struct hit_info& info, color& attenuation, ray& scattered) const;

        color albedo;
        double fuzz;
};

#endif // !__METAL_HPP_
