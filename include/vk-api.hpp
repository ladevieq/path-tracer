#ifndef __VK_API_HPP_
#define __VK_API_HPP_

#include <vector>
#include <optional>
#include <string>

#include "vk-context.hpp"

struct Buffer {
    VmaAllocation       alloc;
    VmaAllocationInfo   alloc_info;
    VkBuffer            handle;
    size_t              size;
};

struct Pipeline {
    std::vector<VkShaderModule> shader_modules;
    VkDescriptorSetLayout       descriptor_set_layout;
    VkPipelineLayout            layout;
    VkPipeline                  handle;
};

struct Image {
    VkImage                 handle;
    VkImageView             view;
    VmaAllocation           alloc;
    VmaAllocationInfo       alloc_info;
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

// TODO: Make the api manage ressources
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

        VkSampler create_sampler(VkFilter filter, VkSamplerAddressMode address_mode);
        void destroy_sampler(VkSampler sampler);


        std::vector<VkCommandBuffer> create_command_buffers(size_t command_buffers_count);
        void destroy_command_buffers(std::vector<VkCommandBuffer> &command_buffers);


        VkRenderPass create_render_pass(std::vector<VkFormat>& color_attachments_format);
        VkRenderPass create_render_pass(std::vector<VkFormat>& color_attachments_format, VkFormat depth_attachement_format);
        void destroy_render_pass(VkRenderPass render_pass);


        VkFramebuffer create_framebuffer(VkRenderPass render_pass, std::vector<Image>& images, VkExtent2D size);
        std::vector<VkFramebuffer> create_framebuffers(VkRenderPass render_pass, std::vector<Image>& images, VkExtent2D size, uint32_t framebuffer_count);
        void destroy_framebuffer(VkFramebuffer framebuffer);
        void destroy_framebuffers(std::vector<VkFramebuffer>& framebuffers);


        Pipeline create_compute_pipeline(const char* shader_name, std::vector<VkDescriptorSetLayoutBinding>& bindings);
        Pipeline create_graphics_pipeline(const char* shader_name, std::vector<VkDescriptorSetLayoutBinding>& bindings, VkShaderStageFlagBits shader_stages, VkRenderPass render_pass, std::vector<VkDynamicState> dynamic_states);
        void destroy_pipeline(Pipeline &pipeline);


        std::vector<VkDescriptorSet> create_descriptor_sets(VkDescriptorSetLayout descriptor_sets_layout, size_t descriptor_sets_count);
        void destroy_descriptor_sets(std::vector<VkDescriptorSet> &descriptor_sets);


        VkSurfaceKHR create_surface(window& wnd);
        void destroy_surface(VkSurfaceKHR surface);


        Swapchain create_swapchain(VkSurfaceKHR surface, size_t min_image_count, VkImageUsageFlags usages, std::optional<std::reference_wrapper<Swapchain>> old_swapchain);
        void destroy_swapchain(Swapchain& swapchain);

        void update_descriptor_set_buffer(VkDescriptorSet set, VkDescriptorSetLayoutBinding binding, Buffer& buffer);
        void update_descriptor_set_buffer(VkDescriptorSet set, VkDescriptorSetLayoutBinding binding, Buffer& buffer, VkDeviceSize offset, VkDeviceSize range);
        void update_descriptor_set_image(VkDescriptorSet set, VkDescriptorSetLayoutBinding binding, VkImageView view, VkSampler sampler);


        void start_record(VkCommandBuffer command_buffer);

        void image_barrier(VkCommandBuffer command_buffer, VkImageLayout src_layout, VkImageLayout dst_layout, VkPipelineStageFlagBits src_stage, VkPipelineStageFlagBits dst_stage, VkAccessFlags src_access, VkAccessFlags dst_access, Image image);

        void begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D size, Pipeline pipeline);
        void end_render_pass(VkCommandBuffer command_buffer);

        void run_compute_pipeline(VkCommandBuffer command_buffer, Pipeline pipeline, VkDescriptorSet set, size_t group_count_x, size_t group_count_y, size_t group_count_z);

        void draw(VkCommandBuffer command_buffer, Pipeline pipeline, VkDescriptorSet set, uint32_t vertex_count, uint32_t vertex_offset);
        void draw(VkCommandBuffer command_buffer, Pipeline pipeline, VkDescriptorSet set, Buffer index_buffer, uint32_t primitive_count, uint32_t indices_offset, uint32_t vertices_offset);

        void blit_full(VkCommandBuffer command_buffer, Image src_image, Image dst_image);

        void end_record(VkCommandBuffer command_buffer);

        VkResult submit(VkCommandBuffer command_buffer, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, VkFence submission_fence);

        VkResult present(Swapchain& swapchain, uint32_t image_index, VkSemaphore wait_semaphore);
    // private:

        vkcontext           context;

    private:

        const char* shader_stage_extension(VkShaderStageFlags shader_stage);

        VkCommandPool       command_pool;
        VkDescriptorPool    descriptor_pool;

};

#endif // !__VK_API_HPP_
