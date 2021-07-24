#ifndef __UTILS_HPP_
#define __UTILS_HPP_

#include <string>
#include <vector>

std::vector<uint8_t> read_file(const char* path);
std::vector<uint8_t> read_file(std::string& path);

#define PI 3.14159265359

inline float deg_to_rad(float deg) {
    return deg * PI / 180.0;
}

inline float randd() {
    return rand() / (RAND_MAX + 1.0);
}

inline float randd(float min, float max) {
    return min + randd() * (max - min);
}

inline int32_t randi(int32_t min, int32_t max) {
    return static_cast<int32_t>(randd(min, max + 1));
}

inline float clamp(float x, float min, float max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

#endif // !__UTILS_HPP_
