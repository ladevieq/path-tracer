#include "color.hpp"
#include "rt.hpp"

void write_color(std::ostream& out, const color& pixel_color, uint32_t samples_per_pixel) {
    auto scale = 1.0 / samples_per_pixel;
    auto scaled_color = pixel_color * scale;

    out << static_cast<uint32_t>(256.0 * clamp(scaled_color.x, 0.0, 0.999)) << ' '
        << static_cast<uint32_t>(256.0 * clamp(scaled_color.y, 0.0, 0.999)) << ' '
        << static_cast<uint32_t>(256.0 * clamp(scaled_color.z, 0.0, 0.999)) << std::endl;
}
