#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "handle.hpp"

struct texture_desc {
    uint32_t            width = 1U;
    uint32_t            height = 1U;
    uint32_t            depth = 1U;
    uint32_t            mips = 1U;
    VkImageUsageFlags   usages;
    VkFormat            format;
    VkImageType         type;

    static constexpr auto max_mips = 16U;
};

struct device_texture;

class texture {
public:
    texture(handle<device_texture> handle);

private:
    handle<device_texture> handle;
};
