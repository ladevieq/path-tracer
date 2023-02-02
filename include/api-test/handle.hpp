#pragma once

#include <cstdint>

template<typename T>
struct handle {
    uint32_t id = handle::invalid_handle;

    [[nodiscard]] inline bool is_valid() const { return id != handle::invalid_handle; }

    static constexpr uint32_t invalid_handle = -1;
};
