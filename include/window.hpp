#ifndef __WINDOW_HPP_
#define __WINDOW_HPP_

#include "defines.hpp"
#include <cstdint>

#if defined(LINUX)
#include <xcb/xcb.h>

class window {
    public:
        window(uint32_t width, uint32_t height);

        xcb_connection_t*   connection;
        xcb_window_t        win;

        uint32_t            width;
        uint32_t            height;

        bool                isOpen;
};
#elif defined(WINDOWS)
#include <Windows.h>

class window {
    public:
        window(uint32_t width, uint32_t height);

        static LRESULT CALLBACK window_procedure(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

        HWND                win_handle;

        uint32_t            width;
        uint32_t            height;

        bool                isOpen;
};
#endif

#endif // !__WINDOW_HPP_
