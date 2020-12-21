#ifndef __RAY_HPP_
#define __RAY_HPP_

#include "vec3.hpp"

class ray {
    public:

        ray();
        ray(const point3& origin, const vec3& direction);

        point3 at(double t) const;

        point3 origin;
        vec3 direction;
};

#endif // !__RAY_HPP_
