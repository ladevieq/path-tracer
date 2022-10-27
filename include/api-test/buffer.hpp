#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "handle.hpp"

struct buffer_desc {
    size_t                  size;
    VkBufferUsageFlags      usages;
    VkMemoryPropertyFlags   memory_properties;
    uint32_t                memory_usage;
};

struct device_buffer;

class buffer {
public:
    buffer(handle<device_buffer> handle);

private:
    handle<device_buffer> handle;

};
