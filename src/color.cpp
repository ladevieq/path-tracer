#include "color.hpp"
#include "rt.hpp"

void write_color(std::ostream& out, const color& pixel_color, uint32_t samples_per_pixel) {
    auto r = pixel_color.x;
    auto g = pixel_color.y;
    auto b = pixel_color.z;

    // auto scale = 1.0 / samples_per_pixel;
    // r = sqrt(r * scale);
    // g = sqrt(g * scale);
    // b = sqrt(b * scale);

    out << static_cast<uint32_t>(256.0 * clamp(r, 0.0, 0.999)) << ' '
        << static_cast<uint32_t>(256.0 * clamp(g, 0.0, 0.999)) << ' '
        << static_cast<uint32_t>(256.0 * clamp(b, 0.0, 0.999)) << std::endl;
}
