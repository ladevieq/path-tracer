#ifndef __VK_API_HPP_
#define __VK_API_HPP_

#include <vector>

#include "vk-context.hpp"

struct Buffer {
    VmaAllocation   alloc;
    VkBuffer        vkbuffer;
    void*           mapped_ptr;
};

struct ComputePipeline {
    VkShaderModule          shader_module;
    VkDescriptorSetLayout   descriptor_set_layout;
    VkPipelineLayout        layout;
    VkPipeline              vkpipeline;
};

class vkapi {
    public:
        vkapi();
        ~vkapi();

        Buffer create_buffer(size_t data_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage mem_usage);
        void destroy_buffer(Buffer& buffer);

        VkFence create_fence();
        void destroy_fence(VkFence fence);
        std::vector<VkFence> create_fences(size_t fences_count);
        void destroy_fences(std::vector<VkFence> fences);

        VkSemaphore create_semaphore();
        void destroy_semaphore(VkSemaphore semaphore);
        std::vector<VkSemaphore> create_semaphores(size_t semaphores_count);
        void destroy_semaphores(std::vector<VkSemaphore> &semaphores);

        std::vector<VkCommandBuffer> create_command_buffers(size_t command_buffers_count);
        void destroy_command_buffers(std::vector<VkCommandBuffer> command_buffers);

        ComputePipeline create_compute_pipeline(const char* shader_path, std::vector<VkDescriptorSetLayoutBinding> bindings);
        void destroy_compute_pipeline(ComputePipeline &pipeline);

        std::vector<VkDescriptorSet> create_descriptor_sets(VkDescriptorSetLayout descriptor_sets_layout, size_t descriptor_sets_count);
        void destroy_descriptor_sets(std::vector<VkDescriptorSet> descriptor_sets);


    // private:

        vkcontext           context;

        VkCommandPool       command_pool;
        VkDescriptorPool    descriptor_pool;

};

#endif // !__VK_API_HPP_
