#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "handle.hpp"

struct device_texture;

class texture {
public:
    texture(handle<device_texture> handle);

private:
    handle<device_texture> handle;
};
