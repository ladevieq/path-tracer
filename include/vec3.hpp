#ifndef __VEC3_HPP_
#define __VEC3_HPP_

#include <iostream>
#include <cmath>

class vec3 {
    public:
    vec3();

    vec3(double x, double y, double z);

    vec3 operator-() const;

    vec3& operator+=(vec3 vec);

    vec3& operator*=(double scale);

    vec3& operator/=(double numerator);

    double length() const;

    double length_sq() const;

    double dot(const vec3& vec) const;

    vec3 cross(const vec3& vec) const;

    vec3 unit() const;

    bool near_zero() const;

    static vec3 random();

    static vec3 random(double min, double max);

    float x, y, z;
    float padding;
};

vec3 lerp(vec3 u, vec3 v, double t);

vec3 operator+(vec3 u, vec3 v);

vec3 operator-(vec3 u, vec3 v);

vec3 operator*(vec3 u, vec3 v);

vec3 operator*(vec3 u, double scale);

vec3 operator*(double scale, vec3 u);

vec3 operator/(vec3 u, double scale);

vec3 reflect(vec3 v, vec3 n);

vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat);

std::ostream& operator<<(std::ostream& out, vec3 vec);

vec3 random_in_unit_sphere();

vec3 random_unit_vector();

vec3 random_in_hemisphere(const vec3& normal);

vec3 random_in_unit_disk();

using point3 = vec3;
using color = vec3;

#endif // !__VEC3_HPP_
