#include "window.hpp"

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
}
#elif defined(WINDOWS)
window::window(uint32_t width, uint32_t height)
    :width(width), height(height) {
}
#endif
