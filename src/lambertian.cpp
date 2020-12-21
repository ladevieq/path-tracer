#include "lambertian.hpp"
#include "hittable.hpp"

lambertian::lambertian(const color& albedo) : albedo(albedo) {}

bool lambertian::scatter(const ray& r, const struct hit_info& info, color& attenuation, ray& scattered) const {
    // auto scatter_direction = info.normal + random_in_unit_sphere();
    // auto scatter_direction = info.normal + random_unit_vector();
    auto scatter_direction = info.normal + random_in_hemisphere(info.normal);

    if (scatter_direction.near_zero()) {
        scatter_direction = info.normal;
    }

    scattered = ray { info.point, scatter_direction };
    attenuation = albedo;
    return true;
}
