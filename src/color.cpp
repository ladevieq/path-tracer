#include "color.hpp"

void write_color(std::ostream& out, const color& pixel_color) {
    out << static_cast<uint32_t>(255.0 * pixel_color.x) << ' '
        << static_cast<uint32_t>(255.0 * pixel_color.y) << ' '
        << static_cast<uint32_t>(255.0 * pixel_color.z) << std::endl;
}
