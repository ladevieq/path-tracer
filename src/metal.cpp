#include "metal.hpp"
#include "hittable.hpp"
#include "vec3.hpp"

metal::metal(const color& albedo, double f)
    : albedo(albedo), fuzz(f < 1 ? f : 1) {}

bool metal::scatter(const ray& r, const struct hit_info& info, color& attenuation, ray& scattered) const {
    auto reflected = reflect(r.direction.unit(), info.normal);

    scattered = ray { info.point, reflected + fuzz * random_in_unit_sphere() };
    attenuation = albedo;
    return (scattered.direction.dot(info.normal) > 0.0);
}
