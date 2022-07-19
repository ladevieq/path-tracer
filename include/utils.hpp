#pragma once

#include <vector>
#include <functional>
#include <unordered_map>
#include <string>

std::vector<uint8_t> read_file(const char* path);

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

#include <windows.h>

#if defined(WINDOWS)
void log_last_error();
#endif

class watcher {
public:
    static bool watch_file(std::filesystem::path&& filepath, std::function<void()> callback);

    static bool watch_dir(std::filesystem::path&& dir_path);

    static void pull_changes();

private:

    struct watch_data {
        HANDLE      dir_handle;
        OVERLAPPED  overlapped;
        DWORD       buffer[512];
    };

    inline static std::unordered_map<std::string, std::function<void()>> callbacks;
    inline static std::unordered_map<std::string, watch_data> watched_dirs;
};
