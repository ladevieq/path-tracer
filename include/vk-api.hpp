#ifndef __VK_API_HPP_
#define __VK_API_HPP_

#include <cassert>
#include <vector>

#include <vulkan/vulkan_core.h>

using VmaAllocation = struct VmaAllocation_T*;

struct buffer {
    VmaAllocation alloc;
    void* device_ptr;
    VkBuffer handle;
    size_t size;
    VkDeviceAddress device_address;
};

struct pipeline {
    std::vector<VkShaderModule> shader_modules;
    VkPipeline handle = VK_NULL_HANDLE;
    VkPipelineBindPoint bind_point;
};

using bindless_index = uint32_t;
using handle = uint32_t;

struct image {
    VkImage handle;
    VkImageView view;
    VmaAllocation alloc;
    VkImageSubresourceRange subresource_range;
    VkFormat format;
    VkExtent3D size;
    VkImageUsageFlags usages;
    bindless_index bindless_storage_index = 0;
    bindless_index bindless_sampled_index = 0;

    VkImageLayout previous_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags previous_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags previous_access = 0;
};

struct swapchain {
    uint32_t image_count;
    std::vector<handle> images;
    VkSwapchainKHR handle;
    VkSurfaceFormatKHR surface_format;
    VkExtent2D extent;
};

struct framebuffer {
    VkFramebuffer handle;
    VkExtent2D size;
};

struct sampler {
    VkSampler handle;
    bindless_index bindless_index;
};

struct global_descriptor {
    VkDescriptorSetLayout set_layout;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSet set;

    std::vector<uint32_t> index_pool[3];

    static constexpr uint32_t pool_size = 1024U;

    global_descriptor() {
        for (int32_t index = pool_size; index > 0; index--) {
            index_pool[0].push_back((uint32_t)index);
            index_pool[1].push_back((uint32_t)index);
            index_pool[2].push_back((uint32_t)index);
        }
    }

    uint32_t allocate(VkDescriptorType type) {
        auto& pool = index_pool[descriptor_type_binding(type)];
        auto index = pool.back();
        pool.pop_back();

        return index;
    }

    void free(uint32_t index, VkDescriptorType type) {
        assert(index != 0);
        index_pool[descriptor_type_binding(type)].push_back(index);
    }

    static int32_t descriptor_type_binding(VkDescriptorType type) {
        switch (type) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            return 0;
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return 1;
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return 2;
        default:
            assert(true && "descriptor type not supported");
            return -1;
        }
    }
};

class window;
class vkcontext;

class vkapi {
    public:
    vkapi(vkcontext& context);
    ~vkapi();

    handle create_buffer(size_t data_size, VkBufferUsageFlags buffer_usage, uint32_t mem_usage);
    void copy_buffer(VkCommandBuffer cmd_buf, handle src, handle dst, size_t size);
    void copy_buffer(VkCommandBuffer cmd_buf, handle src, handle dst, size_t size, size_t buffer_offset = 0);
    void destroy_buffer(handle buffer);


    handle create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usages);
    void destroy_image(handle image);
    std::vector<handle> create_images(VkExtent3D size, VkFormat format, VkImageUsageFlags usages, size_t image_count);
    void destroy_images(std::vector<handle>& imgs);


    [[nodiscard]] VkFence create_fence() const;
    void destroy_fence(VkFence fence) const;
    [[nodiscard]] std::vector<VkFence> create_fences(size_t fences_count) const;
    void destroy_fences(VkFence fences[], size_t fences_count) const;


    [[nodiscard]] VkSemaphore create_semaphore() const;
    void destroy_semaphore(VkSemaphore semaphore) const;
    [[nodiscard]] std::vector<VkSemaphore> create_semaphores(size_t semaphores_count) const;
    void destroy_semaphores(VkSemaphore semaphores[], size_t semaphores_count) const;

    handle create_sampler(VkFilter filter, VkSamplerAddressMode address_mode);
    void destroy_sampler(handle sampler);


    [[nodiscard]] VkCommandPool create_command_pool() const;

    void allocate_command_buffers(VkCommandPool command_pool, VkCommandBuffer* command_buffers, size_t count) const;


    VkRenderPass create_render_pass(std::vector<VkFormat>& color_attachments_format, VkImageLayout initial_layout, VkImageLayout final_layout);
    VkRenderPass create_render_pass(std::vector<VkFormat>& color_attachments_format, VkFormat depth_attachement_format);
    void destroy_render_pass(VkRenderPass render_pass);


    framebuffer create_framebuffer(VkRenderPass render_pass, std::vector<VkFormat>& formats, VkExtent2D size);
    std::vector<framebuffer> create_framebuffers(VkRenderPass render_pass, std::vector<VkFormat>& formats, VkExtent2D size, uint32_t framebuffer_count);
    void destroy_framebuffer(framebuffer framebuffer) const;
    void destroy_framebuffers(std::vector<framebuffer>& framebuffers) const;


    pipeline create_compute_pipeline(const char* shader_name) const;
    pipeline create_graphics_pipeline(const char* shader_name, VkShaderStageFlagBits shader_stages, VkRenderPass render_pass, std::vector<VkDynamicState>& dynamic_states) const;
    void destroy_pipeline(pipeline& pipeline) const;


    VkSurfaceKHR create_surface(window& wnd) const;
    void destroy_surface(VkSurfaceKHR surface);


    swapchain create_swapchain(VkSurfaceKHR surface, size_t min_image_count, VkImageUsageFlags usages, VkSwapchainKHR old_swapchain);
    void destroy_swapchain(swapchain& swapchain);

    void update_descriptor_image(handle img, VkDescriptorType type);
    void update_descriptor_images(std::vector<handle>& images, VkDescriptorType type);
    void update_descriptor_sampler(handle sampler);
    void update_descriptor_samplers(std::vector<handle>& samplers);


    void update_constants(VkCommandBuffer command_buffer, VkShaderStageFlagBits shader_stage, off_t offset, size_t size, void* data);

    void start_record(VkCommandBuffer command_buffer);

    void image_barrier(VkCommandBuffer command_buffer, VkImageLayout dst_layout, VkPipelineStageFlagBits dst_stage, VkAccessFlags dst_access, handle image);

    void begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, std::vector<VkImageView>& attachments, framebuffer framebuffer, VkExtent2D size);
    void end_render_pass(VkCommandBuffer command_buffer);

    void run_compute_pipeline(VkCommandBuffer command_buffer, pipeline& pipeline, size_t group_count_x, size_t group_count_y, size_t group_count_z);

    void draw(VkCommandBuffer command_buffer, pipeline& pipeline, uint32_t vertex_count, uint32_t vertex_offset);
    void draw(VkCommandBuffer command_buffer, pipeline& pipeline, handle index_buffer, uint32_t primitive_count, uint32_t indices_offset, uint32_t vertices_offset);

    void blit_full(VkCommandBuffer command_buffer, handle src_image, handle dst_image);

    void end_record(VkCommandBuffer command_buffer);

    VkResult submit(VkCommandBuffer command_buffers[], size_t command_buffers_count, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, VkFence submission_fence) const;

    VkResult present(swapchain& swapchain, uint32_t image_index, VkSemaphore wait_semaphore) const;

    const buffer& get_buffer(handle buffer_handle) { return *buffers[buffer_handle]; };

    const image& get_image(handle image_handle) { return *images[image_handle]; };

    const sampler& get_sampler(handle sampler_handle) { return *samplers[sampler_handle]; };

    vkcontext& context;

    private:
    global_descriptor bindless_descriptor;

    static const char* shader_stage_extension(VkShaderStageFlags shader_stage);

    // TODO: Use freelists instead
    std::vector<buffer*> buffers;
    std::vector<image*> images;
    std::vector<sampler*> samplers;

    VkDescriptorPool descriptor_pool;
};

#endif // !__VK_API_HPP_
