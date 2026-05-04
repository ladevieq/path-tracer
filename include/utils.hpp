#pragma once

static constexpr size_t KB = 1024U;
static constexpr size_t MB = 1024U * KB;
static constexpr size_t GB = 1024U * MB;
const char* DUMMY_NAME = "NO_NAME";

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

uint8_t* read_file(const char* path, size_t size);

const uint32_t max_markers = 1024U;
// hashmap ?
uint32_t marker_count = 0U;
const char* marker_names[max_markers] = { 0 };
uint64_t marker_counter_start[max_markers] = { 0 };
uint64_t marker_counter_end[max_markers] = { 0 };

#define BEGIN_MARKER(name)                                                      \
        static uint32_t marker_##name##_index = UINT32_MAX;                     \
    do {                                                                        \
        if (marker_##name##_index == UINT32_MAX) {                              \
            marker_##name##_index = marker_count++;                             \
            marker_names[marker_##name##_index] = #name;                        \
        }                                                                       \
        {                                                                       \
            LARGE_INTEGER start_qpc;                                            \
            QueryPerformanceCounter(&start_qpc);                                \
            marker_counter_start[marker_##name##_index] = start_qpc.QuadPart;   \
        }                                                                       \
    } while(0);

#define END_MARKER(name)                                                    \
    do {                                                                    \
        LARGE_INTEGER end_qpc;                                              \
        QueryPerformanceCounter(&end_qpc);                                  \
        marker_counter_end[marker_##name##_index] = end_qpc.QuadPart;       \
    } while(0);

#define PI 3.14159265359
#define EPSILON 0.000001


#define ALIGN(value, alignment) \
    (((value) + ((alignment) - 1)) & ~((alignment) - 1))

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


#if _WIN32

void log_last_error();

typedef void(*func)(void);
bool watch_file(const char* filepath, func callback);

bool watch_dir(const char* dir_path);

void pull_changes();
#endif
