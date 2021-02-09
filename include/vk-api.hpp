#ifndef __VK_API_HPP_
#define __VK_API_HPP_

#include <vector>
#include <optional>

#include "vk-context.hpp"

struct Buffer {
    VmaAllocation   alloc;
    VkBuffer        handle;
    void*           mapped_ptr;
};

struct ComputePipeline {
    VkShaderModule          shader_module;
    VkDescriptorSetLayout   descriptor_set_layout;
    VkPipelineLayout        layout;
    VkPipeline              handle;
};

struct Image {
    VkImage                 handle;
    VkImageView             view;
    VmaAllocation           alloc;
    VkImageSubresourceRange subresource_range;
    VkFormat                format;
    VkExtent3D              size;
};

struct Swapchain {
    size_t                      image_count;
    std::vector<Image>          images;
    VkSwapchainKHR              handle;
    VkSurfaceFormatKHR          surface_format;
    VkExtent2D                  extent;
};

class window;

class vkapi {
    public:
        vkapi();
        ~vkapi();

        Buffer create_buffer(size_t data_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage mem_usage);
        void destroy_buffer(Buffer& buffer);


        Image create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usages);
        void destroy_image(Image &image);
        std::vector<Image> create_images(VkExtent3D size, VkFormat format, VkImageUsageFlags usages, size_t image_count);
        void destroy_images(std::vector<Image> &images);


        VkFence create_fence();
        void destroy_fence(VkFence fence);
        std::vector<VkFence> create_fences(size_t fences_count);
        void destroy_fences(std::vector<VkFence> &fences);


        VkSemaphore create_semaphore();
        void destroy_semaphore(VkSemaphore semaphore);
        std::vector<VkSemaphore> create_semaphores(size_t semaphores_count);
        void destroy_semaphores(std::vector<VkSemaphore> &semaphores);


        std::vector<VkCommandBuffer> create_command_buffers(size_t command_buffers_count);
        void destroy_command_buffers(std::vector<VkCommandBuffer> &command_buffers);


        ComputePipeline create_compute_pipeline(const char* shader_path, std::vector<VkDescriptorSetLayoutBinding> bindings);
        void destroy_compute_pipeline(ComputePipeline &pipeline);


        std::vector<VkDescriptorSet> create_descriptor_sets(VkDescriptorSetLayout descriptor_sets_layout, size_t descriptor_sets_count);
        void destroy_descriptor_sets(std::vector<VkDescriptorSet> &descriptor_sets);


        VkSurfaceKHR create_surface(window& wnd);
        void destroy_surface(VkSurfaceKHR surface);


        Swapchain create_swapchain(VkSurfaceKHR surface, size_t min_image_count, VkImageUsageFlags usages, std::optional<Swapchain> old_swapchain);
        void destroy_swapchain(Swapchain swapchain);

        void update_descriptor_set_buffer(VkDescriptorSet set, VkDescriptorSetLayoutBinding binding, Buffer& buffer);
        void update_descriptor_set_image(VkDescriptorSet set, VkDescriptorSetLayoutBinding binding, VkImageView view);


        void start_record(VkCommandBuffer command_buffer);

        void image_barrier(VkCommandBuffer command_buffer, VkImageLayout src_layout, VkImageLayout dst_layout, VkPipelineStageFlagBits src_stage, VkPipelineStageFlagBits dst_stage, VkAccessFlags src_access, VkAccessFlags dst_access, Image image);

        void run_compute_pipeline(VkCommandBuffer command_buffer, ComputePipeline pipeline, VkDescriptorSet set, size_t group_count_x, size_t group_count_y, size_t group_count_z);

        void blit_full(VkCommandBuffer command_buffer, Image src_image, Image dst_image);

        void end_record(VkCommandBuffer command_buffer);

        VkResult submit(VkCommandBuffer command_buffer, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, VkFence submission_fence);

        VkResult present(Swapchain swapchain, uint32_t image_index, VkSemaphore wait_semaphore);
    // private:

        vkcontext           context;

        VkCommandPool       command_pool;
        VkDescriptorPool    descriptor_pool;

};

#endif // !__VK_API_HPP_
