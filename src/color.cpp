#include "color.hpp"

void write_color(std::ostream& out, color pixel_color) {
    out << pixel_color.x << ' '
        << pixel_color.y << ' '
        << pixel_color.z << ' ';
}
