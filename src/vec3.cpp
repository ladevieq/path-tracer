#include <climits>
#include <cmath>
#include <smmintrin.h>

#include "utils.hpp"
#include "vec3.hpp"

vec3::vec3() : v(_mm_setzero_ps()) {}

vec3::vec3(float x) : v(_mm_set1_ps(x)) {}

vec3::vec3(float x, float y, float z) : v(_mm_set_ps(0.f, z, y, x)) {}

vec3::vec3(__m128& v) : v(v) {}

vec3::vec3(__m128&& v) : v(v) {}

vec3::vec3(const vec3& vec) { v = _mm_load_ps((float*)&vec.v); }

vec3::vec3(const vec3&& vec) noexcept { v = _mm_load_ps((float*)&vec.v); }

vec3 vec3::operator-() const { return { _mm_xor_ps(v, _mm_set1_ps(-0.0f)) }; }

vec3& vec3::operator=(const vec3& vec) {
    v = _mm_load_ps((float*)&vec.v);

    return *this;
}

vec3& vec3::operator+=(const vec3& vec) {
    v = _mm_add_ps(v, vec.v);

    return *this;
}

vec3& vec3::operator*=(float scale) {
    __m128 s = _mm_set1_ps(scale);
    v = _mm_mul_ps(v, s);

    return *this;
}

vec3& vec3::operator/=(float numerator) {
    __m128 s = _mm_set1_ps(numerator);
    v = _mm_div_ps(v, s);

    return *this;
}

float vec3::operator[](const int axis) {
    if (axis > 2) {
        return std::numeric_limits<float>::infinity();
    }

    return v[axis];
}

float vec3::length() const {
    __m128 t = _mm_dp_ps(v, v, 0b11110001);
    return std::sqrt(t[0]);
}

float vec3::length_sq() const { return (_mm_dp_ps(v, v, 0b11110001))[0]; }

float vec3::dot(const vec3& vec) const { return (_mm_dp_ps(v, vec.v, 0b11110001))[0]; }

vec3 vec3::cross(const vec3& vec) const {
    __m128 tmp0 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 tmp1 = _mm_shuffle_ps(vec.v, vec.v, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 tmp2 = _mm_mul_ps(tmp0, vec.v);
    __m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
    __m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
    return _mm_sub_ps(tmp3, tmp4);
}

vec3 vec3::min(const vec3& vec) const {
    return { _mm_min_ps(v, vec.v) };
}

vec3 vec3::max(const vec3& vec) const {
    return { _mm_max_ps(v, vec.v) };
}

vec3& vec3::normalize() {
    v = _mm_div_ps(v, _mm_sqrt_ps(_mm_dp_ps(v, v, 0xff)));

    return *this;
}

bool vec3::near_zero() const {
    // __m128 e = _mm_set1_ps(EPSILON);
    // _mm_cmplt_ps(_mm_andnot_ps(_mm_set1_ps(-0.0f), v), e);
    return (fabs(v[0]) < EPSILON) && (fabs(v[1]) < EPSILON) && (fabs(v[2]) < EPSILON);
}

void vec3::print() const {
    printf("%f %f %f\n", v[0], v[1], v[2]);
}

vec3 vec3::random() { return { randd(), randd(), randd() }; }

vec3 vec3::random(float min, float max) { return { randd(min, max), randd(min, max), randd(min, max) }; }

// TODO: Use simd instructions
vec3 lerp(const vec3& u, const vec3& v, float t) { return (1.f - t) * u + t * v; }

vec3 operator+(const vec3& u, const vec3& v) { return vec3(_mm_add_ps(u.v, v.v)); }

vec3 operator-(const vec3& u, const vec3& v) { return vec3(_mm_sub_ps(u.v, v.v)); }

vec3 operator*(const vec3& u, const vec3& v) { return vec3(_mm_mul_ps(u.v, v.v)); }

vec3 operator*(const vec3& u, float scale) {
    __m128 s = _mm_set1_ps(scale);
    return vec3(_mm_mul_ps(u.v, s));
}

vec3 operator*(float scale, const vec3& u) {
    __m128 s = _mm_set1_ps(scale);
    return vec3(_mm_mul_ps(u.v, s));
}

vec3 operator/(const vec3& u, float scale) {
    __m128 s = _mm_set1_ps(scale);
    return vec3(_mm_div_ps(u.v, s));
}

// TODO: Use simd instructions
vec3 reflect(const vec3& v, const vec3& n) { return v - 2 * v.dot(n) * n; }

// TODO: Use simd instructions
vec3 refract(const vec3& uv, const vec3& n, float etai_over_etat) {
    auto cos_theta = fmin(n.dot(-uv), 1.f);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    vec3 r_out_parallel = -sqrt(fabs(1.f - r_out_perp.length_sq())) * n;

    return r_out_perp + r_out_parallel;
}
