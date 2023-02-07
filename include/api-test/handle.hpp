#pragma once

#include <cstdint>

template<typename T>
struct handle {
    uint32_t id = handle::invalid_id;

    [[nodiscard]] inline bool is_valid() const { return id != handle::invalid_id; }

    static constexpr uint32_t invalid_id = -1U;
};
