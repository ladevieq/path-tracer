#ifndef __VK_API_HPP_
#define __VK_API_HPP_

#include <vector>
#include <map>
#include <iostream>

#include "vk-context.hpp"

struct buffer {
    VmaAllocation       alloc;
    VmaAllocationInfo   alloc_info;
    VkBuffer            handle;
    size_t              size;
    VkDeviceAddress     device_address;
};

struct pipeline {
    std::vector<VkShaderModule> shader_modules;
    VkPipeline                  handle;
    VkPipelineBindPoint         bind_point;
};

struct image {
    VkImage                 handle;
    VkImageView             view;
    VmaAllocation           alloc;
    VmaAllocationInfo       alloc_info;
    VkImageSubresourceRange subresource_range;
    VkFormat                format;
    VkExtent3D              size;
    uint32_t                bindless_storage_index;
    uint32_t                bindless_sampled_index;

    VkImageLayout           previous_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags    previous_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags           previous_access = 0;
};

struct swapchain {
    size_t                  image_count;
    std::vector<image>      images;
    VkSwapchainKHR          handle;
    VkSurfaceFormatKHR      surface_format;
    VkExtent2D              extent;
};

struct sampler {
    VkSampler               handle;
    uint32_t                bindless_index;
};

struct global_descriptor {
    VkDescriptorSetLayout   set_layout;
    VkPipelineLayout        pipeline_layout;
    VkDescriptorSet         set;

    std::vector<uint32_t>   index_pool[3];

    global_descriptor() {
        for (int32_t index = 1024; index >= 0; index--) {
            index_pool[0].push_back((uint32_t)index);
            index_pool[1].push_back((uint32_t)index);
            index_pool[2].push_back((uint32_t)index);
        }
    }

    uint32_t allocate(VkDescriptorType type) {
        auto &pool = index_pool[descriptor_type_binding(type)];
        auto index = pool.back();
        pool.pop_back();

        return index;
    }

    void free(uint32_t index, VkDescriptorType type) {
        index_pool[descriptor_type_binding(type)].push_back(index);
    }

    int32_t descriptor_type_binding(VkDescriptorType type) {
        switch(type) {
            case VK_DESCRIPTOR_TYPE_SAMPLER:
                 return 0;
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                 return 1;
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                return 2;
            default:
                std::cerr << "descriptor type not supported" << std::endl;
                return -1;
        }
    }
};

class window;

class vkapi {
    public:
        vkapi();
        ~vkapi();

        buffer create_buffer(size_t data_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage mem_usage);
        void copy_buffer(VkCommandBuffer cmd_buf, buffer src, buffer dst, size_t size);
        void copy_buffer(VkCommandBuffer cmd_buf, buffer src, image dst);
        void destroy_buffer(buffer& buffer);


        image create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usages);
        void destroy_image(image &image);
        std::vector<image> create_images(VkExtent3D size, VkFormat format, VkImageUsageFlags usages, size_t image_count);
        void destroy_images(std::vector<image> &images);


        VkFence create_fence();
        void destroy_fence(VkFence fence);
        std::vector<VkFence> create_fences(size_t fences_count);
        void destroy_fences(std::vector<VkFence> &fences);


        VkSemaphore create_semaphore();
        void destroy_semaphore(VkSemaphore semaphore);
        std::vector<VkSemaphore> create_semaphores(size_t semaphores_count);
        void destroy_semaphores(std::vector<VkSemaphore> &semaphores);

        sampler create_sampler(VkFilter filter, VkSamplerAddressMode address_mode);
        void destroy_sampler(sampler sampler);


        std::vector<VkCommandBuffer> create_command_buffers(size_t command_buffers_count);
        void destroy_command_buffers(std::vector<VkCommandBuffer> &command_buffers);


        VkRenderPass create_render_pass(std::vector<VkFormat>& color_attachments_format, VkImageLayout initial_layout, VkImageLayout final_layout);
        VkRenderPass create_render_pass(std::vector<VkFormat>& color_attachments_format, VkFormat depth_attachement_format);
        void destroy_render_pass(VkRenderPass render_pass);


        VkFramebuffer create_framebuffer(VkRenderPass render_pass, std::vector<image>& images, VkExtent2D size);
        std::vector<VkFramebuffer> create_framebuffers(VkRenderPass render_pass, std::vector<image>& images, VkExtent2D size, uint32_t framebuffer_count);
        void destroy_framebuffer(VkFramebuffer framebuffer);
        void destroy_framebuffers(std::vector<VkFramebuffer>& framebuffers);


        pipeline create_compute_pipeline(const char* shader_name);
        pipeline create_graphics_pipeline(const char* shader_name, VkShaderStageFlagBits shader_stages, VkRenderPass render_pass, std::vector<VkDynamicState> dynamic_states);
        void destroy_pipeline(pipeline &pipeline);


        std::vector<VkDescriptorSet> create_descriptor_sets(VkDescriptorSetLayout descriptor_sets_layout, size_t descriptor_sets_count);
        void destroy_descriptor_sets(std::vector<VkDescriptorSet> &descriptor_sets);


        VkSurfaceKHR create_surface(window& wnd);
        void destroy_surface(VkSurfaceKHR surface);


        swapchain create_swapchain(VkSurfaceKHR surface, size_t min_image_count, VkImageUsageFlags usages, VkSwapchainKHR old_swapchain);
        void destroy_swapchain(swapchain& swapchain);

        void update_descriptor_image(image &img, VkDescriptorType type);
        void update_descriptor_images(std::vector<image> &images, VkDescriptorType type);
        void update_descriptor_sampler(sampler &sampler);
        void update_descriptor_samplers(std::vector<sampler> &samplers);


        void start_record(VkCommandBuffer command_buffer);

        void image_barrier(VkCommandBuffer command_buffer, VkImageLayout dst_layout, VkPipelineStageFlagBits dst_stage, VkAccessFlags dst_access, image& image);

        void begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D size, pipeline& pipeline);
        void end_render_pass(VkCommandBuffer command_buffer);

        void run_compute_pipeline(VkCommandBuffer command_buffer, pipeline& pipeline, size_t group_count_x, size_t group_count_y, size_t group_count_z);

        void draw(VkCommandBuffer command_buffer, pipeline& pipeline, uint32_t vertex_count, uint32_t vertex_offset);
        void draw(VkCommandBuffer command_buffer, pipeline& pipeline, buffer& index_buffer, uint32_t primitive_count, uint32_t indices_offset, uint32_t vertices_offset);

        void blit_full(VkCommandBuffer command_buffer, image& src_image, image& dst_image);

        void end_record(VkCommandBuffer command_buffer);

        VkResult submit(VkCommandBuffer command_buffer, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, VkFence submission_fence);

        VkResult present(swapchain& swapchain, uint32_t image_index, VkSemaphore wait_semaphore);
    // private:

        vkcontext           context;

        global_descriptor   bindless_descriptor;

    private:

        const char* shader_stage_extension(VkShaderStageFlags shader_stage);

        std::map<VkBuffer, buffer>  buffers;
        std::map<VkImage, image>    images;
        std::map<VkSampler, sampler>samplers;

        VkCommandPool               command_pool;
        VkDescriptorPool            descriptor_pool;

};

#endif // !__VK_API_HPP_
