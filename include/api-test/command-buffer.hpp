#pragma once

#include <cstdint>
#include <span>

#include <vulkan/vulkan_core.h>

#include "handle.hpp"

#include "vec3.hpp"

enum class QueueType : uint32_t {
    GRAPHICS,
    ASYNC_COMPUTE,
    TRANSFER,
    MAX,
};


struct device_texture;
struct device_buffer;
struct device_pipeline;

struct dispatch_params {
    handle<device_pipeline> pipeline;
    vec3u                   group_size;
    vec3u                   local_group_size;
    uint32_t                uniforms_offset;
};

struct renderpass_params {
private:
    struct rect {
        int32_t x;
        int32_t y;
        uint32_t width;
        uint32_t height;
    };

public:
    rect render_area;
    std::span<handle<device_texture>> color_attachments;
};
struct draw_params {
    handle<device_pipeline> pipeline;
    handle<device_buffer>   index_buffer;
    uint32_t vertex_count;
    uint32_t vertex_offset;
    uint32_t index_offset;
    uint32_t instance_count;
    uint32_t uniforms_offset;
};

struct command_buffer {
    void            start() const;
    void            stop() const;

    void            barrier(handle<device_texture> texture_handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access, VkImageLayout layout) const;

    void            copy(handle<device_buffer> buffer_handle, handle<device_texture> texture_handle, VkDeviceSize offset = 0) const;

    VkCommandBuffer vk_command_buffer;
    QueueType       queue_type;
};

// General purpose queue
struct graphics_command_buffer: public command_buffer {
    void dispatch(const dispatch_params& params) const;

    void begin_renderpass(const renderpass_params& params) const;
    void render(const draw_params& params) const;
    void end_renderpass() const;
};

struct compute_command_buffer: public command_buffer {
    void dispatch(const dispatch_params& params);
};

struct transfer_command_buffer: public command_buffer {
};
