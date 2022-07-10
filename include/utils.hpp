#ifndef __UTILS_HPP_
#define __UTILS_HPP_

#include <vector>
#include <functional>

std::vector<uint8_t> read_file(const char* path);
std::vector<uint8_t> read_file(std::string& path);

#define PI 3.14159265359
#define EPSILON 0.000001

namespace std::filesystem {
    class path;
}

inline float deg_to_rad(float deg) {
    return deg * PI / 180.f;
}

inline float randd() {
    return (float)rand() / (RAND_MAX + 1.f);
}

inline float randd(float min, float max) {
    return min + randd() * (max - min);
}

inline int32_t randi(int32_t min, int32_t max) {
    return (int32_t)(randd((float)min, (float)max + 1.f));
}

inline float clamp(float x, float min, float max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

void watch_file(std::filesystem::path&& filepath, std::function<void()>&& callback);

#endif // !__UTILS_HPP_
