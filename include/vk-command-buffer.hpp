#pragma once

#include "handle.hpp"
#include "vk-device-types.hpp"
#include "vec3.hpp"

struct device_texture;
struct device_buffer;
struct device_pipeline;

struct dispatch_params {
    handle<device_pipeline> pipeline;
    vec3u                   group_size;
    vec3u                   local_group_size;
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
    handle<device_texture>* color_attachments;
    size_t                  color_attachments_count;
};

struct draw_params {
    handle<device_pipeline> pipeline;
    handle<device_buffer>   index_buffer;
    uint32_t vertex_count;
    uint32_t vertex_offset;
    uint32_t instance_count;
};

struct draw_indexed_params {
    handle<device_pipeline> pipeline;
    handle<device_buffer>   index_buffer;
    uint32_t vertex_count;
    uint32_t vertex_offset;
    uint32_t index_offset;
    uint32_t instance_count;
};

struct command_buffer {
    void            start() const;
    void            stop() const;

    void            barrier(handle<device_texture> texture_handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access, VkImageLayout layout) const;

    void            copy(handle<device_buffer> buffer_handle, handle<device_texture> texture_handle, uint64_t offset = 0) const;

    VkCommandBuffer vk_command_buffer;
    QueueType       queue_type;
};

// General purpose queue
struct graphics_command_buffer: public command_buffer {
    void dispatch(const dispatch_params& params, const void* data, size_t size) const;

    void begin_renderpass(const renderpass_params& params) const;
    void draw(const draw_params& params, const void* data, size_t size) const;
    void draw_indexed(const draw_indexed_params& params, const void* data, size_t size) const;
    void end_renderpass() const;
};

struct compute_command_buffer: public command_buffer {
    void dispatch(const dispatch_params& params);
};

struct transfer_command_buffer: public command_buffer {
};
