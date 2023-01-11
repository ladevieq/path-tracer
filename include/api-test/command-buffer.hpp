#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "handle.hpp"

enum class QueueType : uint32_t {
    GRAPHICS,
    ASYNC_COMPUTE,
    TRANSFER,
    MAX,
};


struct device_buffer;
struct device_texture;

struct command_buffer {
    void start() const;
    void stop() const;

    void barrier();

    VkCommandBuffer handle;
    QueueType       queue_type;
};

struct graphics_command_buffer : public command_buffer {
    void set_pipeline();

    void dispatch();
};

struct compute_command_buffer : public command_buffer {
    void set_pipeline();

    void dispatch();
};

struct transfer_command_buffer : public command_buffer {
    void copy(::handle<device_buffer> buffer_handle, ::handle<device_texture> texture_handle);
};
