#pragma once

#include <cstdint>

#include "freelist.hpp"

using VmaAllocation = struct VmaAllocation_T*;

enum class QueueType : uint32_t {
    GRAPHICS,
    ASYNC_COMPUTE,
    TRANSFER,
    MAX,
};

struct texture_desc {
    uint32_t              width  = 1U;
    uint32_t              height = 1U;
    uint32_t              depth  = 1U;
    uint32_t              mips   = 1U;
    VkImageUsageFlags     usages;
    VkFormat              format;
    VkImageType           type;
    VkImage               vk_image = nullptr;
    const char*           name = nullptr;

    static constexpr auto max_mips = 16U;
};

struct device_texture {
    void init(const texture_desc& desc);

    void release();

    void                  create_views();

    [[nodiscard]] uint32_t get_storage_index() const {
        assert(storage_index.id != handle<void>::invalid_id);
        return storage_index.id;
    }

    [[nodiscard]] uint32_t get_sampled_index() const {
        assert(sampled_index.id != handle<void>::invalid_id);
        return sampled_index.id;
    }

    VkImage               vk_image                           = nullptr;
    VkImageView           whole_view                         = nullptr;
    VkImageView           mips_views[texture_desc::max_mips] = { nullptr };
    VkImageLayout         layout                             = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags2 stage                              = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags2        access                             = VK_ACCESS_2_NONE;
    VkFormat              format                             = VK_FORMAT_UNDEFINED;
    VkImageAspectFlags    aspects = VK_IMAGE_ASPECT_NONE_KHR;

    uint32_t              width         = 1U;
    uint32_t              height        = 1U;
    uint32_t              depth         = 1U;
    uint32_t              mips          = 1U;

    const char*           name = nullptr;

    private:
    handle<void>          storage_index;
    handle<void>          sampled_index;

    VmaAllocation         alloc         = nullptr;

    static idlist<> storage_indices;
    static idlist<> sampled_indices;
};


struct sampler_desc {
    VkFilter                mag_filter          = VK_FILTER_NEAREST;
    VkFilter                min_filter          = VK_FILTER_NEAREST;
    VkSamplerMipmapMode     mipmap_mode         = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkSamplerAddressMode    address_mode_u      = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode    address_mode_v      = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode    address_mode_w      = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    float                   mip_lod_bias        = 0.f;
    VkBool32                anisotropy          = VK_FALSE;
    float                   max_anisotropy      = 0.f;
    VkBool32                compare_enabled     = VK_FALSE;
    VkCompareOp             compare_op          = VK_COMPARE_OP_NEVER;
    float                   min_lod             = 0.f;
    float                   max_lod             = 0.f;
    VkBorderColor           border_color        = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    VkBool32                unnomalized_coords  = VK_FALSE;
};

struct device_sampler {
    void init(const sampler_desc& desc);

    VkSampler vk_sampler = nullptr;

    [[nodiscard]] uint32_t get_sampler_index() const {
        return sampler_index.id;
    }

    private:
    handle<void> sampler_index;

    static idlist<> sampler_indices;
};


struct buffer_desc {
    size_t                size;
    VkBufferUsageFlags    usages;
    VkMemoryPropertyFlags memory_properties;
    uint32_t              memory_usage;
    const char*           name;
};

struct device_buffer {
    template<typename T>
    T* allocate() {
        u32 alloc_size = sizeof(T);
        if (allocated_size + alloc_size > size) {
            reset();
        }

        offset = allocated_size;
        T* addr = (T*)(static_cast<uint8_t*>(mapped_ptr) + offset);
        allocated_size += alloc_size;
        return addr;
    }

    void* allocate(u32 alloc_size) {
        if (allocated_size + alloc_size > size) {
            reset();
        }

        offset = allocated_size;
        void* addr = (void*)(static_cast<uint8_t*>(mapped_ptr) + offset);
        allocated_size += alloc_size;
        return addr;
    }

    void reset() {
        allocated_size = 0U;
        offset = 0U;
    }

    u64                    offset = 0U;
    u64                    allocated_size = 0U;
    VkBuffer               vk_buffer;
    VmaAllocation          alloc;
    VkDeviceAddress        device_address = 0U;
    void*                  mapped_ptr;
    size_t                 size;
    VkBufferUsageFlagBits2 usages;

    const char*            name;
};

struct graphics_pipeline_desc {
    uint8_t*            vs_code;
    size_t              vs_code_size;
    uint8_t*            fs_code;
    size_t              fs_code_size;
    VkFormat*           color_attachments_format;
    size_t              color_attachments_format_count;
    VkFormat            depth_attachment_format;
    VkFormat            stencil_attachment_format;
    const char*         name;
};

struct compute_pipeline_desc {
    uint8_t*            cs_code;
    size_t              cs_code_size;
    const char*         name;
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
    VkSemaphore vk_semaphore = nullptr;
    uint64_t    value;
};


typedef struct HWND__* HWND;

struct surface_desc {
    HWND                      window_handle;
    VkSurfaceFormatKHR        surface_format;
    VkPresentModeKHR          present_mode;
    VkImageUsageFlags         usages;
    uint32_t                  image_count         = surface_desc::default_image_count;

    static constexpr uint32_t max_image_count     = 3U;
    static constexpr uint32_t default_image_count = 3U;
};

struct device_surface {
    VkSurfaceKHR            vk_surface;
    VkSwapchainKHR          vk_swapchain;
    VkSurfaceFormatKHR      surface_format;

    handle<device_texture>  swapchain_images[surface_desc::max_image_count];
    uint32_t                image_index;
    uint32_t                image_count;
    uint32_t                frame_index = 0U;

    device_semaphore        acquire_semaphores[surface_desc::max_image_count];
    device_semaphore        submit_semaphores[surface_desc::max_image_count];

    uint32_t                acquire_image_index();
};
