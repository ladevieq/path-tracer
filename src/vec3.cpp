#include "vec3.hpp"

vec3::vec3(): x(0.0), y(0.0), z(0.0) {}

vec3::vec3(double x, double y, double z): x(x), y(y), z(z) {}

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

vec3& vec3::operator*=(double scale) {
    x *= scale;
    y *= scale;
    z *= scale;

    return *this;
}

vec3& vec3::operator/=(double numerator) {
    x /= numerator;
    y /= numerator;
    z /= numerator;

    return *this;
}

double vec3::length() const {
    return sqrt(length_sq());
}

double vec3::length_sq() const {
    return x * x + y * y + z * z;
}

double vec3::dot(const vec3& vec) const {
    return x * vec.x + y * vec.y + z * vec.z;
}

vec3 vec3::cross(const vec3& vec) const {
    return vec3(
        y * vec.z - z * vec.y,
        z * vec.x - x * vec.z,
        x * vec.y - y * vec.y
    );
}

vec3 vec3::unit() const {
    return *this / length();
}

vec3 lerp(vec3 u, vec3 v, double t) {
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

vec3 operator*(vec3 u, double scale) {
    return vec3(
        u.x * scale,
        u.y * scale,
        u.z * scale
    );
}

vec3 operator*(double scale, vec3 u) {
    return u * scale;
}

vec3 operator/(vec3 u, double scale) {
    return (1.0 / scale) * u;
}

std::ostream& operator<<(std::ostream& out, vec3 vec) {
    return out << vec.x << ' ' << vec.y << ' ' << vec.z << std::endl;
}
