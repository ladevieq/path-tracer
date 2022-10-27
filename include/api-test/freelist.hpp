#include <numeric>
#include <vector>

template<typename T, const size_t initial_size = 1024U>
class freelist {
    public:
    freelist()
            : holes(initial_size),
              items(initial_size) {
        std::iota(holes.begin(), holes.end(), 0);
    }

    uint32_t add(const T& item) {
        const auto id = holes.back();
        holes.pop_back();

        items[id] = std::move(item);

        return id;
    }

    void remove(uint32_t item_id) {
        holes.push_back(item_id);
    }

    const T& operator[](uint32_t index) {
        return items[index];
    }

    std::vector<T>        items;
    std::vector<uint32_t> holes;
};
