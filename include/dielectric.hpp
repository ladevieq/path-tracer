#ifndef __DIELECTRIC_HPP_
#define __DIELECTRIC_HPP_

#include "material.hpp"

class dielectric : public material {
    public:
        dielectric(double index_of_refraction);

        virtual bool scatter(const ray& r, const struct hit_info& info, color& attenuation, ray& scattered) const;

        double ior;

    private:
        static double reflectance(double cosine, double ref_idx);
};

#endif // !__DIELECTRIC_HPP_
