#ifndef __VEC3_HPP_
#define __VEC3_HPP_

#include <iostream>
#include <cmath>

class vec3 {
    public:
    vec3();

    vec3(double x, double y, double z);

    void operator-();

    vec3& operator+=(vec3& vec);

    vec3& operator*=(double scale);

    vec3& operator/=(double numerator);

    double length();

    double length_sq();

    double dot(const vec3& vec) const;

    vec3 cross(const vec3& vec) const;

    vec3 unit();

    double x, y, z;
};

vec3 lerp(vec3 u, vec3 v, double t);

vec3 operator+(vec3 u, vec3 v);

vec3 operator-(vec3 u, vec3 v);

vec3 operator*(vec3 u, vec3 v);

vec3 operator*(vec3 u, double scale);

vec3 operator*(double scale, vec3 u);

vec3 operator/(vec3 u, double scale);

std::ostream& operator<<(std::ostream& out, vec3 vec);

using point3 = vec3;
using color = vec3;

#endif // !__VEC3_HPP_
