#pragma once
#include <numeric>
#include <vector>
#include <cassert>

static constexpr uint32_t default_element_count = 1024U;

template<typename T, const size_t initial_size = default_element_count>
class freelist {
    public:
    freelist()
            : free(initial_size),
              items(initial_size) {
        std::iota(free.rbegin(), free.rend(), 0);
    }

    uint32_t add(const T& item) {
        const auto id = free.back();
        free.pop_back();

        items[id] = item;

        return id;
    }

    void remove(uint32_t item_id) {
        free.push_back(item_id);
    }

    T& operator[](uint32_t index) {
        assert(index >= 0);
        return items[index];
    }

    const T& operator[](uint32_t index) const {
        return items[index];
    }

    uint32_t find(const T& seeked_item) {
        for (auto index { 0U }; index < items.size(); index++) {
            const auto& item = items[index];
            if (memcmp(&item, &seeked_item, sizeof(T)) == 0) {
                return index;
            }
        }

        return invalid_id;
    }

    auto begin() const {
        return items.begin();
    }

    auto end() const {
        return items.end();
    }

    static constexpr uint32_t invalid_id = -1;

private:
    std::vector<T>        items;
    std::vector<uint32_t> free;
};
