#include "vec3.hpp"
#include "rt.hpp"

vec3::vec3(): x(0.0), y(0.0), z(0.0) {}

vec3::vec3(float x, float y, float z): x(x), y(y), z(z) {}

vec3 vec3::operator-() const {
    return vec3 {
        -x,
        -y,
        -z
    };
}

vec3& vec3::operator+=(vec3 vec) {
    x += vec.x;
    y += vec.y;
    z += vec.z;

    return *this;
}

vec3& vec3::operator*=(float scale) {
    x *= scale;
    y *= scale;
    z *= scale;

    return *this;
}

vec3& vec3::operator/=(float numerator) {
    x /= numerator;
    y /= numerator;
    z /= numerator;

    return *this;
}

float vec3::length() const {
    return sqrt(length_sq());
}

float vec3::length_sq() const {
    return x * x + y * y + z * z;
}

float vec3::dot(const vec3& vec) const {
    return x * vec.x + y * vec.y + z * vec.z;
}

vec3 vec3::cross(const vec3& vec) const {
    return vec3(
        y * vec.z - z * vec.y,
        z * vec.x - x * vec.z,
        x * vec.y - y * vec.x
    );
}

vec3 vec3::unit() const {
    return *this / length();
}

bool vec3::near_zero() const {
    const auto s = 1e-8;

    return (fabs(x) < s) && (fabs(y) < s) && (fabs(z) < s);
}

vec3 vec3::random() {
    return vec3 { randd(), randd(), randd() };
}

vec3 vec3::random(float min, float max) {
    return vec3 { randd(min, max), randd(min, max), randd(min, max) };
}

vec3 lerp(vec3 u, vec3 v, float t) {
    return (1.0 - t) * u + t * v;
}

vec3 operator+(vec3 u, vec3 v) {
    return vec3(u.x + v.x, u.y + v.y, u.z + v.z);
}

vec3 operator-(vec3 u, vec3 v) {
    return vec3(u.x - v.x, u.y - v.y, u.z - v.z);
}

vec3 operator*(vec3 u, vec3 v) {
    return vec3(u.x * v.x, u.y * v.y, u.z * v.z);
}

vec3 operator*(vec3 u, float scale) {
    return vec3(
        u.x * scale,
        u.y * scale,
        u.z * scale
    );
}

vec3 operator*(float scale, vec3 u) {
    return u * scale;
}

vec3 operator/(vec3 u, float scale) {
    return (1.0 / scale) * u;
}

vec3 reflect(vec3 v, vec3 n) {
    return v - 2 * v.dot(n) * n;
}

vec3 refract(const vec3& uv, const vec3& n, float etai_over_etat) {
    auto cos_theta = fmin(n.dot(-uv), 1.0);
    vec3 r_out_perp =  etai_over_etat * (uv + cos_theta*n);
    vec3 r_out_parallel = -sqrt(fabs(1.0 - r_out_perp.length_sq())) * n;

    return r_out_perp + r_out_parallel;
}

std::ostream& operator<<(std::ostream& out, vec3 vec) {
    return out << vec.x << ' ' << vec.y << ' ' << vec.z << std::endl;
}

// TODO: Use something else to find random point onto a sphere
vec3 random_in_unit_sphere() {
    while (true) {
        auto p = vec3::random(-1,1);
        if (p.length_sq() >= 1) continue;
        return p;
    }
}

vec3 random_unit_vector() {
    return random_in_unit_sphere().unit();
}

vec3 random_in_hemisphere(const vec3& normal) {
    vec3 in_unit_sphere = random_in_unit_sphere();
    if (in_unit_sphere.dot(normal) > 0.0) // In the same hemisphere as the normal
        return in_unit_sphere;
    else
        return -in_unit_sphere;
}

vec3 random_in_unit_disk() {
    while (true) {
        auto p = vec3(randd(-1, 1), randd(-1, 1), 0);
        if (p.length_sq() >= 1) continue;
        return p;
    }
}
