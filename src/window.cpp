#include "window.hpp"

window::window() {
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
        400,
        400,
        10,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen_info->root_visual,
        0,
        nullptr
    );

    xcb_map_window(connection, win);
    xcb_flush(connection);
}
