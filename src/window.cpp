#include "window.hpp"

#ifdef LINUX
#include <iostream>

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

#elif _WIN32
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

HCURSOR window::cursors[CURSOR_TYPE::MAX_CURSOR] = {
    LoadCursorA(NULL, IDC_ARROW),
    LoadCursorA(NULL, IDC_IBEAM),
    LoadCursorA(NULL, IDC_SIZEALL),
    LoadCursorA(NULL, IDC_SIZENS),
    LoadCursorA(NULL, IDC_SIZEWE),
    LoadCursorA(NULL, IDC_SIZENESW),
    LoadCursorA(NULL, IDC_SIZENWSE),
    LoadCursorA(NULL, IDC_HAND),
    LoadCursorA(NULL, IDC_WAIT),
    LoadCursorA(NULL, IDC_APPSTARTING),
    LoadCursorA(NULL, IDC_NO),
};

LRESULT CALLBACK window::window_procedure(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    window* win = nullptr;

    if (umsg == WM_CREATE) {
        auto *lpcs = (LPCREATESTRUCT)lparam;
        win = (window*)lpcs->lpCreateParams;

        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)win);
    } else {
        win = (window*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
        return win->message_handler(hwnd, umsg, wparam, lparam);
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}

void message_loop(void* wnd) {
    ((window*)wnd)->message_loop();
}

window::window(uint32_t desired_width, uint32_t desired_height)
    : window_class_name("default-window"), width(desired_width), height(desired_height) {
    auto *process_handle = GetModuleHandleA(nullptr);

    WNDCLASS            window_class {};
    window_class.style          = 0;
    window_class.lpfnWndProc    = &window::window_procedure;
    window_class.cbClsExtra     = 0;
    window_class.cbWndExtra     = 0;
    window_class.hInstance      = process_handle;
    window_class.hIcon          = nullptr;
    window_class.hCursor        = nullptr;
    window_class.hbrBackground  = nullptr;
    window_class.lpszMenuName   = nullptr;
    window_class.lpszClassName  = window_class_name;

    RegisterClass(&window_class);

    RECT rect { 0, 0, (int32_t)desired_width, (int32_t)desired_height };
    DWORD style = WS_OVERLAPPEDWINDOW;

    if (FAILED(AdjustWindowRect(&rect, style, FALSE))) {
        log_last_error();
    }

    main_fiber = ConvertThreadToFiber(nullptr);
    message_loop_fiber = CreateFiber(0, &::message_loop, this);

    handle = CreateWindowA(
        "default-window",
        "path-tracer",
        style,
        10,
        10,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        process_handle,
        this
    );

    ShowWindow(handle, SW_SHOW);

    isOpen = true;
}

window::~window() {
    DestroyWindow(handle);
    UnregisterClassA(window_class_name, GetModuleHandleA(nullptr));
}

void window::message_loop() {
    MSG msg {};

    while(true) {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE) != 0) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        for (u32 key_index = 0U; key_index <= KEY::MAX_KEY; key_index++) {
            events.push_back(event {
                .data {
                    .key = (KEY)key_index,
                },
                .type = keyboard[key_index] ? EVENT_TYPE::KEY_DOWN : EVENT_TYPE::KEY_UP,
            });
        }

        SwitchToFiber(main_fiber);
    }
}

void window::poll_events() {
    events.clear();

    SwitchToFiber(message_loop_fiber);
}

LRESULT window::message_handler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    switch(umsg) {
        case WM_CLOSE:
        case WM_DESTROY:
        case WM_QUIT: {
            events.push_back({
                .data {},
                .type = EVENT_TYPE::QUIT
            });
            isOpen = false;
            break;
        }
        case WM_SIZE: {
            width = LOWORD(lparam);
            height = HIWORD(lparam);

            events.push_back({
                .data {
                    .size {
                        .width = width,
                        .height = height,
                    },
                },
                .type = EVENT_TYPE::RESIZE
            });
            break;
        }

        case WM_KEYDOWN: {
            events.push_back({ 
                .data {
                    .key = (KEY)wparam,
                },
                .type = EVENT_TYPE::KEY_PRESS,
            });
            keyboard[wparam] = true;
            break;
        }

        case WM_KEYUP: {
            events.push_back({ 
                .data {
                    .key = (KEY)wparam,
                },
                .type = EVENT_TYPE::KEY_RELEASE,
            });
            keyboard[wparam] = false;
            break;
        }

        case WM_LBUTTONDOWN: {
            mouse[BUTTON::LEFT] = true;
            events.push_back({ 
                .data {
                    .button = BUTTON::LEFT,
                },
                .type = EVENT_TYPE::BUTTON_PRESS,
            });
            break;
        }
        case WM_LBUTTONUP: {
            mouse[BUTTON::LEFT] = false;
            events.push_back({ 
                .data {
                    .button = BUTTON::LEFT,
                },
                .type = EVENT_TYPE::BUTTON_RELEASE,
            });
            break;
        }
        case WM_RBUTTONDOWN: {
            mouse[BUTTON::RIGHT] = true;
            events.push_back({ 
                .data {
                    .button = BUTTON::RIGHT,
                },
                .type = EVENT_TYPE::BUTTON_PRESS,
            });
            break;
        }
        case WM_RBUTTONUP: {
            mouse[BUTTON::RIGHT] = false;
            events.push_back({ 
                .data {
                    .button = BUTTON::RIGHT,
                },
                .type = EVENT_TYPE::BUTTON_RELEASE,
            });
            break;
        }
        case WM_MBUTTONDOWN: {
            mouse[BUTTON::MIDDLE] = true;
            events.push_back({ 
                .data {
                    .button = BUTTON::MIDDLE,
                },
                .type = EVENT_TYPE::BUTTON_PRESS,
            });
            break;
        }
        case WM_MBUTTONUP: {
            mouse[BUTTON::MIDDLE] = false;
            events.push_back({ 
                .data {
                    .button = BUTTON::MIDDLE,
                },
                .type = EVENT_TYPE::BUTTON_RELEASE,
            });
            break;
        }
        case WM_MOUSEMOVE: {
            events.push_back({ 
                .data {
                    .position {
                        .x = GET_X_LPARAM(lparam),
                        .y = GET_Y_LPARAM(lparam),
                   },
                },
                .type = EVENT_TYPE::MOUSE_MOVE,
            });
            break;
        }
        case WM_MOUSEWHEEL: {
            events.push_back({ 
                .data {
                    .delta {
                        .x = 0,
                        .y = GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA,
                   },
                },
                .type = EVENT_TYPE::MOUSE_WHEEL,
            });
            break;
        }
        default: {
            return DefWindowProc(hwnd, umsg, wparam, lparam);
        }
    }

    return 0;
}

void window::set_cursor(enum CURSOR_TYPE type) {
    SetCursor(window::cursors[type]);
}
#elif defined(MACOS)
window::window(uint32_t width, uint32_t height) {
}

void window::poll_events() {}
#endif
