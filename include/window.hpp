#ifndef __WINDOW_HPP_
#define __WINDOW_HPP_

#include <cstdint>

#if defined(__linux__)
#define LINUX
#include <unistd.h>
#include <dlfcn.h>
#elif defined(_WIN32) || defined(_WIN64)
#define WINDOWS
#include <Windows.h>
#endif

#if defined(LINUX)
#include <xcb/xcb.h>

class window {
    public:
        window(uint32_t width, uint32_t height);

        xcb_connection_t*   connection;
        xcb_window_t        win;

        uint32_t            width;
        uint32_t            height;
};
#elif defined(WINDOWS)
#include <Windows.h>

class window {
    public:
        window(uint32_t width, uint32_t height);

        uint32_t            width;
        uint32_t            height;
};
#endif

#endif // !__WINDOW_HPP_
