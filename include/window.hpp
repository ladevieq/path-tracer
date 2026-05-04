#ifndef __WINDOW_HPP_
#define __WINDOW_HPP_

#if defined(LINUX)
#include <xcb/xcb.h>

#include <xkbcommon/xkbcommon.h>
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
#if _WIN32
enum KEY: uint32_t {
    LSHIFT = 0x10,
    RSHIFT = 0x10,
    LCTRL = 0x11,
    RCTRL = 0x11,
    ALT,
    SPACE = 0x20,
    PAGE_UP,
    PAGE_DOWN,
    END,
    HOME,
    LEFT_ARROW,
    UP_ARROW,
    RIGHT_ARROW,
    DOWN_ARROW,
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
    F1 = 0x70,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    MAX_KEY = 0xff
};
#else
enum KEY: uint32_t {
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
    MAX_KEY = 0xffff
};
#endif

enum BUTTON: uint32_t {
    LEFT,
    RIGHT,
    MIDDLE,
    MAX_BUTTON = 255
};

enum EVENT_TYPE: int32_t {
    QUIT,
    RESIZE,
    KEY_PRESS,
    KEY_DOWN,
    KEY_RELEASE,
    KEY_UP,
    BUTTON_PRESS,
    BUTTON_RELEASE,
    MOUSE_MOVE,
    MOUSE_WHEEL,
};

enum CURSOR_TYPE: int32_t {
    ARROW,
    TEXT_INPUT,
    SIZE_ALL,
    SIZE_NS,
    SIZE_WE,
    SIZE_NESW,
    SIZE_NWSE,
    HAND,
    WAIT,
    PROGRESS,
    NOT_ALLOWED,
    MAX_CURSOR,
};

// struct event {
//     uint32_t    width;
//     uint32_t    height;
//     KEYS        key;
//     BUTTONS     button;
//     int32_t     x;
//     int32_t     y;
//     EVENT_TYPES type;
// };
struct event {
    union {
        struct {
            uint32_t    width;
            uint32_t    height;
        } size;
        struct {
            int32_t     x;
            int32_t     y;
        } position;
        struct {
            int32_t     x;
            int32_t     y;
        } delta;
        KEY        key;
        BUTTON     button;
    } data;
    EVENT_TYPE type;
};

class window {
    public:
        window(uint32_t desired_width, uint32_t desired_height);
        ~window();

        void poll_events();

#if _WIN32
        void message_loop();

        LRESULT message_handler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK window_procedure(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

        static void set_cursor(enum CURSOR_TYPE type);

        HWND                handle {};
        const char*         window_class_name;
        LPVOID              main_fiber;
        LPVOID              message_loop_fiber;
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

        bool keyboard[KEY::MAX_KEY] { false };
        bool mouse[BUTTON::MAX_BUTTON] { false };

        static HCURSOR cursors[CURSOR_TYPE::MAX_CURSOR];
};

#endif // !__WINDOW_HPP_
