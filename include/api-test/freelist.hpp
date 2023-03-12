#pragma once
#include <numeric>
#include <vector>
#include <cassert>

#include "handle.hpp"

static constexpr uint32_t default_element_count = 1024U;

template<typename T, const size_t initial_size = default_element_count>
class freelist {
    public:

    freelist() {
        grow(initial_size);
    }

    handle<T> add(const T& item) {
        auto id = elements[0U].next_free;

        if (id == invalid_id) {
            grow(elements.size());

            id = elements[0U].next_free;
        }

        auto& element = elements[id];

        elements[0U].next_free = element.next_free;
        element.item = std::move(item);

        return { id, ++element.generation };
    }

    void remove(handle<T> handle) {
        assert(handle.generation == elements[handle.id].generation);
        elements[handle.id].next_free = elements[0U].next_free;
        elements[0U].next_free = handle.id;
    }

    T& operator[](handle<T> handle) {
        assert(handle.generation == elements[handle.id].generation);
        return elements[handle.id].item;
    }

    const T& operator[](handle<T> handle) const {
        assert(handle.generation == elements[handle.id].generation);
        return elements[handle.id].item;
    }

    static constexpr uint32_t invalid_id = -1;

private:
    void grow(size_t grow_size) {
        auto size = elements.size();
        uint32_t next_free = size;

        elements.reserve(size + grow_size);

        for (auto id { 0U }; id < initial_size; id++) {
            elements.push_back(
                {
                    .next_free = static_cast<uint32_t>(++next_free % initial_size),
                }
            );
        }

        elements.back().next_free = -1U;
    }

    struct element {
        union {
            T item;
            uint32_t next_free = -1;
        };
        uint32_t generation = 0U;
    };

    std::vector<element>        elements;
};

template<const size_t initial_size = default_element_count>
class idlist {
    public:

    idlist() {
        grow(initial_size);
    }

    handle<void> add() {
        auto id = elements[0U].next_free;

        if (id == invalid_id) {
            grow(elements.size());

            id = elements[0U].next_free;
        }

        auto& element = elements[id];
        elements[0U].next_free = element.next_free;
        element.next_free = -1;

        return { id, ++element.generation };
    }

    void remove(handle<void> handle) {
        assert(handle.generation == elements[handle.id].generation);
        elements[handle.id].next_free = elements[0U].next_free;
        elements[0U].next_free = handle.id;
    }

    static constexpr uint32_t invalid_id = -1;

private:
    void grow(size_t grow_size) {
        auto size = elements.size();
        uint32_t next_free = size;

        elements.reserve(size + grow_size);

        for (auto id { 0U }; id < initial_size; id++) {
            elements.push_back(
                {
                    .next_free = static_cast<uint32_t>(++next_free % initial_size),
                }
            );
        }

        elements.back().next_free = -1U;
    }

    struct element {
        uint32_t next_free = -1;
        uint32_t generation = 0U;
    };

    std::vector<element>        elements;
};
