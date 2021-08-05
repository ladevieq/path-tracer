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

// enum MODIFIERS: uint32_t {
//     SHIFT       = 1,
//     LOCK        = 2,
//     CTRL        = 4,
//     ALT         = 8,
//     MOD2        = 16,
//     MOD3        = 32,
//     MOD4        = 64,
//     MOD5        = 128,
//     BUTTON1     = 256,
//     BUTTON2     = 512,
//     BUTTON3     = 1024,
//     BUTTON4     = 2048,
//     BUTTON5     = 4096,
// };

// TODO improve this
#if defined(WINDOWS)
enum KEYS: uint32_t {
    SHIFT = 0x10,
    CTRL,
    ALT,
    SPACE = 0x20,
    A = 0x41,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    LSHIFT = 0xA0,
    RSHIFT,
    MAX_KEYS = 255
};
#else
enum KEYS: uint32_t {
    SPACE = 32,
    A = 0x61,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    LSHIFT = 65505,
    RSHIFT,
    LCTRL,
    RCTRL,
    LALT = 65513,
    RALT,
    MAX_KEYS = 0xffff
};
#endif

enum BUTTONS: uint32_t {
    LEFT,
    RIGHT,
    MIDDLE,
    MAX_BUTTONS = 255
};

enum EVENT_TYPES: int32_t {
    QUIT,
    RESIZE,
    KEY_PRESS,
    KEY_RELEASE,
    BUTTON_PRESS,
    BUTTON_RELEASE,
    MOUSE_MOVE
};

struct event {
    uint32_t    width;
    uint32_t    height;
    KEYS        key;
    BUTTONS     button;
    int32_t     x;
    int32_t     y;
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

        bool keyboard[KEYS::MAX_KEYS] = { false };
        bool mouse[BUTTONS::MAX_BUTTONS] = { false };
};

#endif // !__WINDOW_HPP_
