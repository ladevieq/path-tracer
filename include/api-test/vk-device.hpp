#pragma once

#include <cstdint>
#include <span>

#include <vulkan/vulkan_core.h>

#include "command-buffer.hpp"
#include "freelist.hpp"
#include "handle.hpp"
#include "vk-bindless.hpp"
#include "vk-device-types.hpp"

using VmaAllocator = struct VmaAllocator_T*;

class window;

class vkdevice {
    struct queue {
        VkQueue       vk_queue;
        uint32_t      index;
        VkQueueFlags  usages;
        VkCommandPool command_pool;
    };

    public:
    void init();

    static inline vkdevice& get_render_device() {
        return render_device;
    }

    handle<device_texture>   create_texture(const texture_desc& desc);

    handle<device_buffer>    create_buffer(const buffer_desc& desc);

    handle<device_pipeline>  create_pipeline(const pipeline_desc& desc);

    handle<device_surface>   create_surface(const surface_desc& desc);

    handle<device_semaphore> create_semaphore(const semaphore_desc& desc);

    void                     destroy_texture(const device_texture& texture);
    void                     destroy_texture(handle<device_texture> handle);

    void                     destroy_buffer(const device_buffer& buffer);
    void                     destroy_buffer(handle<device_buffer> handle);

    void                     destroy_pipeline(const device_pipeline& pipeline);
    void                     destroy_pipeline(handle<device_pipeline> handle);

    void                     destroy_semaphore(const device_semaphore& semaphore);
    void                     destroy_semaphore(handle<device_semaphore> handle);

    void                     destroy_surface(const device_surface& surface);
    void                     destroy_surface(handle<device_surface> handle);

    template<typename cmd_buf_type>
    void allocate_command_buffers(cmd_buf_type* buffers, size_t count, QueueType queue_type) {
        allocate_command_buffers(static_cast<command_buffer*>(buffers), count, queue_type);
    }

    [[nodiscard]] inline device_texture& get_texture(handle<device_texture> handle) {
        return textures[handle.id];
    }

    [[nodiscard]] inline const device_buffer& get_buffer(handle<device_buffer> handle) const {
        return buffers[handle.id];
    }

    [[nodiscard]] inline const device_pipeline& get_pipeline(handle<device_pipeline> handle) const {
        return pipelines[handle.id];
    }

    [[nodiscard]] inline const device_surface& get_surface(handle<device_surface> handle) const {
        return surfaces[handle.id];
    }

    [[nodiscard]] inline device_semaphore& get_semaphore(handle<device_semaphore> handle) {
        return semaphores[handle.id];
    }

    [[nodiscard]] inline const bindless_model& get_bindingmodel() const {
        return bindless;
    }

    inline VkDevice get_device() {
        return device;
    }

    void wait(handle<device_semaphore> semaphore_handle);

    void submit(std::span<command_buffer> buffers, handle<device_semaphore> wait_handle, handle<device_semaphore> signal_handle);

    void present(handle<device_surface> surface_handle, handle<device_semaphore> semaphore_handle);

    private:
    vkdevice() = default;
    ~vkdevice();

    void allocate_command_buffers(command_buffer* buffers, size_t count, QueueType type);

    VkShaderModule create_shader_module(const std::span<uint8_t>& shader_code);

    void create_views(const texture_desc& desc, device_texture& texture);

    void create_swapchain(const surface_desc& desc, device_surface& surface);

    // TODO: Wrap in a struct called adapter ?
    void                       create_instance();

    void                       create_device();

    void                       create_memory_allocator();

    void                       create_command_pools();

    void                       pick_physical_device();

    static bool                device_support_features(VkPhysicalDevice physical_dev);

    void                       create_debug_layer_callback();

    queue                      queues[static_cast<uint32_t>(QueueType::MAX)];

    VkInstance                 instance;
    VkPhysicalDevice           physical_device;
    VkDevice                   device;

    VmaAllocator               gpu_allocator;

    bindless_model             bindless;

    static constexpr size_t    max_allocable_command_buffers  = 16;
    static constexpr size_t    max_submitable_command_buffers = 16;

    freelist<device_texture*>  storage_indices;
    freelist<device_texture*>  sampled_indices;

    freelist<device_texture>   textures;
    freelist<device_buffer>    buffers;
    freelist<device_pipeline>  pipelines;
    freelist<device_surface>   surfaces;
    freelist<device_semaphore> semaphores;

    static vkdevice            render_device;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif // _DEBUG
};
