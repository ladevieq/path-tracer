#include "window.hpp"

#include <iostream>

#include "vk-renderer.hpp"
#include "utils.hpp"

#ifdef LINUX
#include <xkbcommon/xkbcommon-x11.h>

window::window(uint32_t width, uint32_t height)
    :width(width), height(height) {

    connection = xcb_connect(NULL, NULL);
    win = xcb_generate_id(connection);

    auto setup = xcb_get_setup (connection);
    auto screen_info = xcb_setup_roots_iterator(setup).data;

    uint32_t events[] = {
        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION,
    };
    xcb_create_window(
        connection,
        XCB_COPY_FROM_PARENT,
        win,
        screen_info->root,
        10,
        10,
        width,
        height,
        10,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen_info->root_visual,
        XCB_CW_EVENT_MASK,
        events
    );

    // Ask for WM_PROTOCOLS atom
    auto protocols_id   = xcb_intern_atom(connection, true, 12, "WM_PROTOCOLS");
    auto protocols_atom = xcb_intern_atom_reply(connection, protocols_id, NULL)->atom;

    // Ask for WM_DELETE_WINDOW atom
    auto win_delete_id  = xcb_intern_atom(connection, false, 16, "WM_DELETE_WINDOW");
    win_delete_atom     = xcb_intern_atom_reply(connection, win_delete_id, NULL)->atom;

    // Allow the window to receive win close events
    // https://lists.freedesktop.org/archives/xcb/2010-December/006713.html
    xcb_change_property(
        connection,
        XCB_PROP_MODE_REPLACE,
        win,
        protocols_atom,
        4,              // Predefined value for atom type
        32,
        1,
        &win_delete_atom
    );

    xcb_map_window(connection, win);
    xcb_flush(connection);

    uint8_t first_xkb_event;
    auto ret = xkb_x11_setup_xkb_extension(connection,
                                      XKB_X11_MIN_MAJOR_XKB_VERSION,
                                      XKB_X11_MIN_MINOR_XKB_VERSION,
                                      XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
                                      NULL, NULL, NULL, NULL);
    if (!ret) {
        std::cerr << "Couldn't setup XKB extension" << std::endl;
    }

    xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    if (!xkb_ctx) {
        std::cerr << "Cannot create xkb context" << std::endl;
    }

    keyboard_device_id = xkb_x11_get_core_keyboard_device_id(connection);

    if (keyboard_device_id == -1) {
        std::cerr << "Cannot access keyboard device !" << std::endl;
        exit(1);
    }

    keyboard_keymap = xkb_x11_keymap_new_from_device(xkb_ctx, connection, keyboard_device_id, XKB_KEYMAP_COMPILE_NO_FLAGS);

    keyboard_state = xkb_x11_state_new_from_device(keyboard_keymap, connection, keyboard_device_id);

    isOpen = true;
}

window::~window() {
    xkb_state_unref(keyboard_state);
    xkb_keymap_unref(keyboard_keymap);
    xkb_context_unref(xkb_ctx);
    xcb_disconnect(connection);
}

void window::poll_events() {
    xcb_generic_event_t *event;
    while((event = xcb_poll_for_event (connection))) {
        switch(event->response_type & ~0x80) {
            case XCB_CONFIGURE_NOTIFY: {
                auto notify_event = (xcb_configure_notify_event_t*)event;

                if (width != notify_event->width || height != notify_event->height) {
                    events.push_back({ 
                        .width  = notify_event->width,
                        .height = notify_event->height,
                        .type   = EVENT_TYPES::RESIZE
                    });

                    width = notify_event->width;
                    height = notify_event->height;
                }

                break;
            }
            case XCB_KEY_RELEASE:
            case XCB_KEY_PRESS: {
                auto key_press_event = (xcb_key_press_event_t*)event;
                auto keysym = xkb_state_key_get_one_sym(keyboard_state, key_press_event->detail);
                auto isKeyPress = event->response_type == XCB_KEY_PRESS;
                auto key = (KEYS)keysym;

                keyboard[key] = isKeyPress;

                struct event new_event = {
                    .key = key,
                    .type = isKeyPress ? EVENT_TYPES::KEY_PRESS :EVENT_TYPES::KEY_RELEASE
                };

                events.push_back(new_event);
                break;
            }
            case XCB_BUTTON_RELEASE:
            case XCB_BUTTON_PRESS: {
                auto key_press_event    = (xcb_key_press_event_t*)event;
                struct event new_event = {
                    .type   = event->response_type == XCB_BUTTON_PRESS ? EVENT_TYPES::BUTTON_PRESS :EVENT_TYPES::BUTTON_RELEASE
                };

                events.push_back(new_event);
                break;
            }
            case XCB_MOTION_NOTIFY: {
                auto motion_event = (xcb_motion_notify_event_t*)event;
                events.push_back({
                    .x      = motion_event->event_x,
                    .y      = motion_event->event_y,
                    .type   = EVENT_TYPES::MOUSE_MOVE,
                });
                break;
            }
            case XCB_CLIENT_MESSAGE: {
                auto client_message_event = (xcb_client_message_event_t*)event;
                if (client_message_event->data.data32[0] == win_delete_atom)  {
                    events.push_back({ .type = EVENT_TYPES::QUIT });
                    isOpen = false;
                }
            }
            default: {
            }
        }

        free(event);
    }
}

#elif defined(WINDOWS)

#include <windowsx.h>
#include <cctype>

LRESULT CALLBACK window::window_procedure(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    window* win = nullptr;

    if (umsg == WM_CREATE) {
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lparam;
        win = (window*)lpcs->lpCreateParams;

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)win);
    } else {
        win = (window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        return win->message_handler(hwnd, umsg, wparam, lparam);
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}

window::window(uint32_t width, uint32_t height)
    :width(width), height(height) {
        auto process_handle = GetModuleHandle(NULL);

        WNDCLASS window_class       = {};
        window_class.style          = 0;
        window_class.lpfnWndProc    = &window::window_procedure;
        window_class.cbClsExtra     = 0;
        window_class.cbWndExtra     = 0;
        window_class.hInstance      = process_handle;
        window_class.hIcon          = NULL;
        window_class.hCursor        = NULL;
        window_class.hbrBackground  = NULL;
        window_class.lpszMenuName   = NULL;
        window_class.lpszClassName  = "default-window";

        RegisterClass(&window_class);

        win_handle = CreateWindow(
            "default-window",
            "path-tracer",
            WS_OVERLAPPEDWINDOW,
            10,
            10,
            width,
            height,
            NULL,
            NULL,
            process_handle,
            this
        );

        ShowWindow(win_handle, SW_SHOW);

        isOpen = true;
}

window::~window() {}

void window::poll_events() {
    MSG msg = {};

    // Generate keydown events pressed keys
    for (size_t virtual_key = KEYS::LSHIFT; virtual_key < KEYS::MAX_KEYS; virtual_key++) {
        if (keyboard[virtual_key]) {
            events.push_back({ 
                .key = (KEYS)virtual_key,
                .type = EVENT_TYPES::KEY_PRESS,
            });
        }
    }


    while (PeekMessage(&msg, win_handle, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT window::message_handler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    switch(umsg) {
        case WM_CLOSE:
        case WM_DESTROY:
        case WM_QUIT: {
            events.push_back({ .type = EVENT_TYPES::QUIT });
            isOpen = false;
            break;
        }
        case WM_SIZE: {
            width = LOWORD(lparam);
            height = HIWORD(lparam);

            events.push_back({
                .width = width,
                .height = height,
                .type = EVENT_TYPES::RESIZE
            });
            break;
        }

        case WM_KEYDOWN: {
            keyboard[wparam] = true;

            events.push_back({ 
                .key = (KEYS)wparam,
                .type = EVENT_TYPES::KEY_PRESS,
            });
            break;
        }

        case WM_KEYUP: {
            keyboard[wparam] = false;

            events.push_back({ 
                .key = (KEYS)wparam,
                .type = EVENT_TYPES::KEY_RELEASE,
            });
            break;
        }

        case WM_LBUTTONDOWN: {
            events.push_back({ 
                .button = BUTTONS::LEFT,
                .type = EVENT_TYPES::BUTTON_PRESS,
            });
            break;
        }
        case WM_LBUTTONUP: {
            events.push_back({ 
                .button = BUTTONS::LEFT,
                .type = EVENT_TYPES::BUTTON_RELEASE,
            });
            break;
        }
        case WM_RBUTTONDOWN: {
            events.push_back({ 
                .button = BUTTONS::RIGHT,
                .type = EVENT_TYPES::BUTTON_PRESS,
            });
            break;
        }
        case WM_RBUTTONUP: {
            events.push_back({ 
                .button = BUTTONS::RIGHT,
                .type = EVENT_TYPES::BUTTON_RELEASE,
            });
            break;
        }
        case WM_MBUTTONDOWN: {
            events.push_back({ 
                .button = BUTTONS::MIDDLE,
                .type = EVENT_TYPES::BUTTON_PRESS,
            });
            break;
        }
        case WM_MBUTTONUP: {
            events.push_back({ 
                .button = BUTTONS::MIDDLE,
                .type = EVENT_TYPES::BUTTON_RELEASE,
            });
            break;
        }
        case WM_MOUSEMOVE: {
            events.push_back({ 
                .x = GET_X_LPARAM(lparam),
                .y = GET_Y_LPARAM(lparam),
                .type = EVENT_TYPES::MOUSE_MOVE,
            });
            break;
        }
        default: {
            return DefWindowProc(hwnd, umsg, wparam, lparam);
        }
    }
    return 0;
}
#elif defined(MACOS)
window::window(uint32_t width, uint32_t height) {
}

window::~window() {};

void window::poll_events() {}
#endif
