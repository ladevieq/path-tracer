#pragma once

#include <cstdint>

template<typename U>
struct handle {
    uint32_t id = invalid_id;
    uint32_t generation = 0U;

    [[nodiscard]] inline bool is_valid() const { return id != handle::invalid_id; }

    static constexpr uint32_t invalid_id = UINT32_MAX;
};
