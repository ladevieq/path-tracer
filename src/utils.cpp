#include "utils.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>

std::vector<uint8_t> get_shader_code(const char* path) {
    FILE* f = fopen(path, "r");

    if (f == nullptr) {
        std::cerr << "Cannot open compute shader file !" << std::endl;
        return {};
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        return {};
    }

    std::vector<uint8_t> shader_code(ftell(f));

    if (fseek(f, 0, SEEK_SET) != 0) {
        return {};
    }

    if (fread(shader_code.data(), sizeof(uint8_t), shader_code.size(), f) != shader_code.size()) {
        std::cerr << "Reading shader code failed" << std::endl;
        return {};
    }

    return shader_code;
}
