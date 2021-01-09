#ifndef __WINDOW_HPP_
#define __WINDOW_HPP_

#include <xcb/xcb.h>

class window {
    public:
        window();

    private:
        xcb_connection_t*   connection;
        xcb_window_t        win;
};

#endif // !__WINDOW_HPP_
