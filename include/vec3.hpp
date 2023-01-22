#ifndef __VEC3_HPP_
#define __VEC3_HPP_

#include <xmmintrin.h>
#include <cstdint>

struct vec3u {
    union {
        uint32_t vec[3];
        uint32_t x, y, z;
    };
};

class vec3 {
  public:
    vec3();

    vec3(float x);
    vec3(float x, float y, float z);
    vec3(__m128& v);
    vec3(__m128&& v);
    vec3(const vec3& vec);
    vec3(const vec3&& vec) noexcept;

    vec3 operator-() const;

    vec3& operator=(const vec3& vec);

    vec3& operator+=(const vec3& vec);

    vec3& operator*=(float scale);

    vec3& operator/=(float numerator);

    vec3 operator<(const vec3& vec) const;

    vec3 operator>(const vec3& vec) const;

    float operator[](int axis);

    [[nodiscard]] float length() const;

    [[nodiscard]] float length_sq() const;

    [[nodiscard]] float dot(const vec3& vec) const;

    [[nodiscard]] vec3 cross(const vec3& vec) const;

    vec3& normalize();

    [[nodiscard]] bool near_zero() const;

    void print() const;

    static vec3 random();

    static vec3 random(float min, float max);

    __m128 v;
};

vec3 lerp(const vec3& u, const vec3& v, float t);

vec3 operator+(const vec3& u, const vec3& v);

vec3 operator-(const vec3& u, const vec3& v);

vec3 operator*(const vec3& u, const vec3& v);

vec3 operator*(const vec3& u, float scale);

vec3 operator*(float scale, const vec3& u);

vec3 operator/(const vec3& u, float scale);

vec3 reflect(const vec3& v, const vec3& n);

vec3 refract(const vec3& uv, const vec3& n, float etai_over_etat);

using point3 = vec3;
using color = vec3;

#endif // !_
