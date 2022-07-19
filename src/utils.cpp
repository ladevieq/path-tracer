#include "utils.hpp"

#if defined(WINDOWS)
#include <windows.h>
#endif

#include <filesystem>
#include <iostream>

std::vector<uint8_t> read_file(const char* path) {
#if defined(WINDOWS)
    FILE* f = nullptr;
    errno_t err = fopen_s(&f, path, "rb");

    if (err != 0) {
        std::cerr << "Cannot open file !" << std::endl;
        return {};
    }
#elif defined(LINUX)
    FILE* f = fopen(path, "rb");

    if (!f) {
        std::cerr << "Cannot open file !" << std::endl;
        return {};
    }
#endif

    if (fseek(f, 0, SEEK_END) != 0) {
        return {};
    }

    std::vector<uint8_t> file_content(ftell(f));

    if (fseek(f, 0, SEEK_SET) != 0) {
        return {};
    }

    if (fread(file_content.data(), sizeof(uint8_t), file_content.size(), f) != file_content.size()) {
        std::cerr << "Reading file content failed" << std::endl;
        return {};
    }

    fclose(f);

    return file_content;
}

std::vector<uint8_t> read_file(std::string& path) {
    return read_file(path.c_str());
}
