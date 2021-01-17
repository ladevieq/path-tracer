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
        0,
        nullptr
    );

    xcb_map_window(connection, win);
    xcb_flush(connection);

    isOpen = true;
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
        case WM_QUIT: {
            isOpen = false;
            break;
        }
        case WM_SIZE: {
            RECT rect;
            if(GetWindowRect(hwnd, &rect))
            {
              width = rect.right - rect.left;
              height = rect.bottom - rect.top;
            }

            if (renderer != nullptr) {
                renderer->recreate_swapchain();
                break;
            }
        }
        default:
            return DefWindowProc(hwnd, umsg, wparam, lparam);
    }

    return 0;
}
#endif
