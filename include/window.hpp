#ifndef __WINDOW_HPP_
#define __WINDOW_HPP_

#include <cstdint>
#include <vector>

#include "defines.hpp"

class vkrenderer;

#if defined(LINUX)
#include <xcb/xcb.h>

#include <xkbcommon/xkbcommon.h>
#elif defined(WINDOWS)
#include <Windows.h>
#endif

enum KEYS {
    SPACE,
    Z,
    Q,
    S,
    D,
};

enum EVENT_TYPES: int32_t {
    QUIT,
    RESIZE,
    KEY_PRESS,
    KEY_RELEASE,
};

struct event {
    uint32_t    width;
    uint32_t    height;
    // KEYS        key;
    uint32_t    keycode;
    EVENT_TYPES type;
};

class window {
    public:
        window(uint32_t width, uint32_t height);
        ~window();

        void poll_events();


#if defined(WINDOWS)
        LRESULT message_handler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK window_procedure(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

        HWND                win_handle;
#elif defined(LINUX)
        xcb_connection_t*   connection;
        xcb_window_t        win;
        xcb_atom_t          win_delete_atom;

        xkb_context*        xkb_ctx;
        int32_t             keyboard_device_id;
        xkb_keymap*         keyboard_keymap;
        xkb_state*          keyboard_state;
#endif

        std::vector<event>  events;

        uint32_t            width;
        uint32_t            height;

        bool                isOpen;
};

#endif // !__WINDOW_HPP_
