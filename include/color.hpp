#ifndef __COLOR_HPP_
#define __COLOR_HPP_

#include <iostream>

#include "vec3.hpp"

void write_color(std::ostream& out, const color& pixel_color, uint32_t samples_per_pixel);

#endif // !__COLOR_HPP_
