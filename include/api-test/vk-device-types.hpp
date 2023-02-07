#pragma once

#include <vulkan/vulkan_core.h>

#include <span>

using VmaAllocation = struct VmaAllocation_T*;

struct texture_desc {
    uint32_t              width  = 1U;
    uint32_t              height = 1U;
    uint32_t              depth  = 1U;
    uint32_t              mips   = 1U;
    VkImageUsageFlags     usages;
    VkFormat              format;
    VkImageType           type;

    static constexpr auto max_mips = 16U;
};

struct device_texture {
    VkImage               vk_image;
    VkImageView           whole_view;
    VkImageView           mips_views[texture_desc::max_mips] = { VK_NULL_HANDLE };
    VkImageLayout         layout;
    VkPipelineStageFlags2 stage         = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags2        access        = VK_ACCESS_2_NONE;
    VkFormat              format;
    VkImageAspectFlags    aspects;

    uint32_t              width  = 1U;
    uint32_t              height = 1U;
    uint32_t              depth  = 1U;
    uint32_t              mips   = 1U;

    uint32_t              storage_index = -1;
    uint32_t              sampled_index = -1;

    VmaAllocation         alloc;
};

struct buffer_desc {
    size_t                size;
    VkBufferUsageFlags    usages;
    VkMemoryPropertyFlags memory_properties;
    uint32_t              memory_usage;
};

struct device_buffer {
    VkBuffer        vk_buffer;
    VmaAllocation   alloc;
    VkDeviceAddress device_address = 0U;
    void*           mapped_ptr;
    size_t          size;
};


struct pipeline_desc {
    std::span<uint8_t>  cs_code;
    std::span<uint8_t>  vs_code;
    std::span<uint8_t>  fs_code;
    std::span<VkFormat> color_attachments_format;
    VkFormat            depth_attachment_format;
    VkFormat            stencil_attachment_format;
};

struct device_pipeline {
    VkPipeline          vk_pipeline;
    VkPipelineBindPoint bind_point;
};


struct semaphore_desc {
    VkSemaphoreType type;
    uint64_t        initial_value = 0U;
};

struct device_semaphore {
    VkSemaphore    vk_semaphore = nullptr;
    uint64_t       value;
};


typedef struct HWND__* HWND;

struct surface_desc {
    HWND                      window_handle;
    VkSurfaceFormatKHR        surface_format;
    VkPresentModeKHR          present_mode;
    VkImageUsageFlags         usages;
    uint32_t                  image_count         = surface_desc::default_image_count;

    static constexpr uint32_t max_image_count = 3U;
    static constexpr uint32_t default_image_count = 3U;
};

struct device_surface {
    VkSurfaceKHR           vk_surface;
    VkSwapchainKHR         vk_swapchain;

    // handle<device_texture> swapchain_images;
    uint32_t               image_index;
    uint32_t               image_count;

    void acquire_image_index(handle<device_semaphore> signal_handle);
    handle<device_texture> get_backbuffer_image();
};
