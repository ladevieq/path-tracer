#pragma once

#include <vector>
#include <unordered_map>

std::vector<uint8_t> read_file(const char* path);

#define PI 3.14159265359
#define EPSILON 0.000001

inline float deg_to_rad(float deg) {
    return static_cast<float>(deg * PI / 180.f);
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
    if (x < min) { return min; }
    if (x > max) { return max; }
    return x;
}


#if defined(WINDOWS)

#include <windows.h>

void log_last_error();

class watcher {
public:
    typedef void(*func)(void);

    static bool watch_file(const char* filepath, func callback);

    static bool watch_dir(const char* dir_path);

    static void pull_changes();

private:

    struct watch_data {
        HANDLE      dir_handle;
        OVERLAPPED  overlapped;
        DWORD       buffer[512];
    };

    inline static std::unordered_map<const char*, func> callbacks;
    inline static std::unordered_map<const char*, watch_data> watched_dirs;
};
#endif
