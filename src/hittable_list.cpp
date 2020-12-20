#include "hittable_list.hpp"

hittable_list::hittable_list() {};

void hittable_list::add(std::shared_ptr<hittable> hittable) {
    hittables.push_back(hittable);
}

void hittable_list::clear() {
    hittables.clear();
}


bool hittable_list::hit(const ray& r, double t_min, double t_max, struct hit_info& info) const {
    struct hit_info temp_info;
    bool hit_anything = false;
    double closest_hit = t_max;

    for (auto& hittable: hittables) {
        if (hittable->hit(r, t_min, closest_hit, temp_info)) {
            hit_anything = true;
            closest_hit = temp_info.t;
            info = temp_info;
        }
    }

    return hit_anything;
}
