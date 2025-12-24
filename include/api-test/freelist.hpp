#pragma once
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
        // auto id = elements[0U].next_free;
        auto id = next_free_id;

        if (id == freelist<T>::invalid_id) {
            grow(elements.size());

            // id = elements[0U].next_free;
            id = next_free_id;
        }

        auto& element = elements[id];

        next_free_id = element.next_free;
        // elements[0U].next_free = element.next_free;
        element.item = std::move(item);

        return { id, ++element.generation };
    }

    void remove(handle<T> handle) {
        assert(handle.generation == elements[handle.id].generation);
        // elements[handle.id].next_free = elements[0U].next_free;
        // elements[0U].next_free = handle.id;
        elements[handle.id].next_free = next_free_id;
        next_free_id = handle.id;
    }

    T& operator[](handle<T> handle) {
        assert(handle.generation == elements[handle.id].generation);
        return elements[handle.id].item;
    }

    const T& operator[](handle<T> handle) const {
        assert(handle.generation == elements[handle.id].generation);
        return elements[handle.id].item;
    }

    static constexpr uint32_t invalid_id = UINT32_MAX;

private:
    void grow(size_t grow_size) {
        auto size = elements.size();
        uint32_t next_free = static_cast<uint32_t>(size);
        next_free_id = next_free;

        elements.reserve(size + grow_size);

        for (auto id { 0U }; id < initial_size; id++) {
            elements.push_back(
                {
                    .next_free = static_cast<uint32_t>(++next_free % initial_size),
                }
            );
        }

        elements.back().next_free = freelist<T>::invalid_id;
    }

    struct element {
        union {
            T item;
            uint32_t next_free = freelist<T>::invalid_id;
        };
        uint32_t generation = 0U;
    };

    std::vector<element>        elements;
    uint32_t next_free_id;
};

template<const size_t initial_size = default_element_count>
class idlist {
    public:

    idlist() {
        grow(initial_size);
    }

    handle<void> add() {
        // auto id = elements[0U].next_free;
        auto id = next_free_id;

        if (id == idlist<T>::invalid_id) {
            grow(elements.size());

            // id = elements[0U].next_free;
            id = next_free_id;
        }

        auto& element = elements[id];
        next_free_id = element.next_free;
        elements[0U].next_free = element.next_free;
        element.next_free = idlist<T>::invalid_id;

        return { id, ++element.generation };
    }

    void remove(handle<void> handle) {
        assert(handle.generation == elements[handle.id].generation);
        // elements[handle.id].next_free = elements[0U].next_free;
        // elements[0U].next_free = handle.id;
        elements[handle.id].next_free = next_free_id;
        next_free_id = handle.id;
    }

    static constexpr uint32_t invalid_id = UINT32_MAX;

private:
    void grow(size_t grow_size) {
        auto size = elements.size();
        uint32_t next_free = static_cast<uint32_t>(size);
        next_free_id = next_free;

        elements.reserve(size + grow_size);

        for (auto id { 0U }; id < initial_size; id++) {
            elements.push_back(
                {
                    .next_free = static_cast<uint32_t>(++next_free% initial_size),
                }
            );
        }

        elements.back().next_free = idlist<T>::invalid_id;
    }

    struct element {
        uint32_t next_free = idlist<T>::invalid_id;
        uint32_t generation = 0U;
    };

    std::vector<element>        elements;
    uint32_t next_free_id;
};
