#ifndef __WINDOW_HPP_
#define __WINDOW_HPP_

#include <xcb/xcb.h>

class window {
    public:
        window(uint32_t width, uint32_t height);

        xcb_connection_t*   connection;
        xcb_window_t        win;

        uint32_t            width;
        uint32_t            height;
};

#endif // !__WINDOW_HPP_
