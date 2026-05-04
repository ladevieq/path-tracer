#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "handle.hpp"

struct device_buffer;

class buffer {
public:
    buffer(handle<device_buffer> handle);

private:
    handle<device_buffer> handle;

};
