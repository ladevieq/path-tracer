#ifndef __HITTABLE_LIST_HPP_
#define __HITTABLE_LIST_HPP_

#include <vector>
#include <memory>

#include "hittable.hpp"

class hittable_list : public hittable {
    public:
        hittable_list();

        void add(std::shared_ptr<hittable> hittable);
        void clear();

        virtual bool hit(const ray&r, double t_min, double t_max, struct hit_info& info) const;

        std::vector<std::shared_ptr<hittable>> hittables;
};

#endif // !__HITTABLE_LIST_HPP_
