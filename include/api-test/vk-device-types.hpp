#pragma once

#include <vulkan/vulkan_core.h>

#include <span>

#include "vk-bindless.hpp"

using VmaAllocation = struct VmaAllocation_T*;

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

struct device_texture {
    VkImage               vk_image;
    VkImageView           whole_view;
    VkImageView           mips_views[texture_desc::max_mips] = { VK_NULL_HANDLE };
    VkImageLayout         layout;
    VkPipelineStageFlags2 stage  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags2        access = VK_ACCESS_2_NONE;

    uint32_t              storage_index = -1;
    uint32_t              sampled_index = -1;
    uint32_t              sampler_index = -1;

    VmaAllocation         alloc;

    texture_desc          desc;
};


struct buffer_desc {
    size_t                  size;
    VkBufferUsageFlags      usages;
    VkMemoryPropertyFlags   memory_properties;
    uint32_t                memory_usage;
};

struct device_buffer {
    VkBuffer        vk_buffer;
    VmaAllocation   alloc;
    VkDeviceAddress device_address = 0U;
    void*           mapped_ptr;

    buffer_desc     desc;
};


struct pipeline_desc {
    std::span<uint8_t> vs_code;
    std::span<uint8_t> fs_code;
    std::span<uint8_t> cs_code;
    std::span<VkFormat> color_attachments_format;
    VkFormat depth_attachment_format;
    VkFormat stencil_attachment_format;
};

struct device_pipeline {
    VkPipeline          vk_pipeline;
    VkPipelineBindPoint bind_point;
    pipeline_desc       desc;

    bindless_model*     bindless = nullptr;
};
