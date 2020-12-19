#include <iostream>

int main() {
    size_t width = 256;
    size_t height = 256;

    // PPM format header
    std::cout << "P3" << std::endl;

    std::cout << width << std::endl;
    std::cout << height << std::endl;

    // Max color
    std::cout << "255" << std::endl;

    std::cerr << "Generating image" << std::endl;

    for (size_t column = 0; column < width; column++) {
        std::cerr << width - column << " lines remaining" << std::endl;

        for (size_t row = 0; row < height; row++) {
            auto r = double(column) / (width-1);
            auto g = double(row) / (height-1);
            auto b = 0.25;

            int ir = static_cast<int>(255.999 * r);
            int ig = static_cast<int>(255.999 * g);
            int ib = static_cast<int>(255.999 * b);

            std::cout << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }

    std::cerr << "Done !" << std::endl;

    return 0;
}
