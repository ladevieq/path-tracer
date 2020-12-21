#include "sphere.hpp"

sphere::sphere(point3 center, double radius, std::shared_ptr<material> material)
    : center(center), radius(radius), mat_ptr(material)
{}

bool sphere::hit(const ray& r, double t_min, double t_max, struct hit_info& info) const {
    // Vector from the ray origin to sphere position
    vec3 origin_center = r.origin - center;

    double a = r.direction.dot(r.direction);
    double half_b = r.direction.dot(origin_center);
    double c = origin_center.dot(origin_center) - radius * radius;
    double discriminant = half_b * half_b - a * c;

    if (discriminant < 0) {
        return false;
    }

    double sq_discriminant = sqrt(discriminant);
    double t = (-half_b - sq_discriminant) / a ;

    if (t_min > t || t_max < t) {
        t = (-half_b + sq_discriminant) / a ;

        if (t_min > t || t_max < t) {
            return false;
        }
    }

    info.t = t;
    info.point = r.at(t);
    info.mat_ptr = mat_ptr;

    vec3 out_normal = (info.point - center).unit();
    info.set_face_normal(r, out_normal);

    return true;
}
