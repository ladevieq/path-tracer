#include "color.hpp"
#include "utils.hpp"

void write_color(std::ostream& out, const color& pixel_color) {
    auto r = pixel_color.v[0];
    auto g = pixel_color.v[1];
    auto b = pixel_color.v[2];

    out << static_cast<uint32_t>(256.0 * clamp(r, 0.0, 0.999)) << ' '
        << static_cast<uint32_t>(256.0 * clamp(g, 0.0, 0.999)) << ' '
        << static_cast<uint32_t>(256.0 * clamp(b, 0.0, 0.999)) << std::endl;
}
