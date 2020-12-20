#include "color.hpp"

void write_color(std::ostream& out, const color& pixel_color, uint32_t samples_per_pixel) {

    auto scale = 1.0 / samples_per_pixel;
    auto scaled_color = pixel_color * scale;

    out << static_cast<uint32_t>(255.0 * scaled_color.x) << ' '
        << static_cast<uint32_t>(255.0 * scaled_color.y) << ' '
        << static_cast<uint32_t>(255.0 * scaled_color.z) << std::endl;
}
