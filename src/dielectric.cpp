#include "dielectric.hpp"
#include "rt.hpp"
#include "hittable.hpp"

dielectric::dielectric(double index_of_refraction)
    : ior(index_of_refraction) {}

bool dielectric::scatter(const ray& r, const struct hit_info& info, color& attenuation, ray& scattered) const {
    auto unit_direction = r.direction.unit();
    auto refraction_ratio = info.front_face ? (1.0 / ior) : ior;
    double cos_theta = fmin(info.normal.dot(-unit_direction), 1.0);
    double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    vec3 direction;
    if (refraction_ratio * sin_theta > 1.0 || reflectance(cos_theta, refraction_ratio) > randd()) {
        direction = reflect(unit_direction, info.normal);
    } else {
        direction = refract(unit_direction, info.normal, refraction_ratio);
    }

    scattered = ray { info.point, direction };
    attenuation = { 1.0, 1.0, 1.0 };

    return true;
}

double dielectric::reflectance(double cosine, double ref_idx) {
    // Use Schlick's approximation for reflectance.
    auto r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow ((1 - cosine), 5);
}
