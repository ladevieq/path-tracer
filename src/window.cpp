#include "window.hpp"
#include "vk-renderer.hpp"
#include "utils.hpp"

#ifdef LINUX
window::window(uint32_t width, uint32_t height)
    :width(width), height(height) {
    connection = xcb_connect(NULL, NULL);
    win = xcb_generate_id(connection);

    auto setup = xcb_get_setup (connection);
    auto screen_info = xcb_setup_roots_iterator(setup).data;

    xcb_event_mask_t events[] = {
        XCB_EVENT_MASK_STRUCTURE_NOTIFY,
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

    isOpen = true;
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
            case XCB_CLIENT_MESSAGE: {
                auto client_message_event = (xcb_client_message_event_t*)event;
                if (client_message_event->data.data32[0] == win_delete_atom)  {
                    events.push_back({ .type = EVENT_TYPES::QUIT });
                }
            }
            default: {
            }
        }

        free(event);
    }
}

#elif defined(WINDOWS)
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

void window::poll_events() {
    MSG msg = {};

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
            isOpen = false;
            return 0;
        }
        case WM_SIZE: {
            width = LOWORD(lparam);
            height = HIWORD(lparam);

            events.push_back({
                .width = width,
                .height = height,
                .type = EVENT_TYPES::RESIZE
            });
            return 0;
        }
    }
    return DefWindowProc(hwnd, umsg, wparam, lparam);
}
#endif
