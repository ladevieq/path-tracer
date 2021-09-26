#include <cassert>
#include <memory>
#include <iostream>

#include "vk-api.hpp"
#include "window.hpp"
#include "utils.hpp"

#ifdef _DEBUG
#define VKRESULT(result) assert(result == VK_SUCCESS);
#else
#define VKRESULT(result) result;
#endif


vkapi::vkapi() {
    VkCommandPoolCreateInfo cmd_pool_create_info    = {};
    cmd_pool_create_info.sType                      = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.pNext                      = nullptr;
    cmd_pool_create_info.flags                      = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_create_info.queueFamilyIndex           = context.queue_index;

    VKRESULT(vkCreateCommandPool(context.device, &cmd_pool_create_info, nullptr, &command_pool))

    std::vector<VkDescriptorPoolSize> descriptor_pools_sizes = {
        {
            .type                               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount                    = (uint32_t)1024,
        },
        {
            .type                               = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount                    = (uint32_t)1024,
        },
        {
            .type                               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount                    = (uint32_t)1024,
        },
        {
            .type                               = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount                    = (uint32_t)1024,
        }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info  = {};
    descriptor_pool_create_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext                       = nullptr;
    descriptor_pool_create_info.flags                       = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_create_info.maxSets                     = 0xff;
    descriptor_pool_create_info.poolSizeCount               = descriptor_pools_sizes.size();
    descriptor_pool_create_info.pPoolSizes                  = descriptor_pools_sizes.data();

    VKRESULT(vkCreateDescriptorPool(context.device, &descriptor_pool_create_info, nullptr, &descriptor_pool))

    // Global descriptor
     VkDescriptorSetLayoutBinding bindless_layout_bindings[] = {
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                .descriptorCount = 1024,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = 1024,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
            {
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1024,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
    };

    VkDescriptorBindingFlags flags[] = {
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
    };
    VkDescriptorSetLayoutBindingFlagsCreateInfo descriptor_set_layout_binding_flags = {};
    descriptor_set_layout_binding_flags.sType           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    descriptor_set_layout_binding_flags.pNext           = VK_NULL_HANDLE;
    descriptor_set_layout_binding_flags.bindingCount    = sizeof(flags) / sizeof(VkDescriptorBindingFlags);
    descriptor_set_layout_binding_flags.pBindingFlags   = flags;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext         = &descriptor_set_layout_binding_flags;
    descriptor_set_layout_create_info.flags         = 0;
    descriptor_set_layout_create_info.bindingCount  = sizeof(bindless_layout_bindings) / sizeof(VkDescriptorSetLayoutBinding);
    descriptor_set_layout_create_info.pBindings     = bindless_layout_bindings;

    VKRESULT(vkCreateDescriptorSetLayout(context.device, &descriptor_set_layout_create_info, nullptr, &bindless_descriptor.set_layout))


    VkPushConstantRange push_constants[] = {
        {
            .stageFlags                        = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset                            = 0,
            .size                              = 64,
        },
        {
            .stageFlags                        = VK_SHADER_STAGE_VERTEX_BIT,
            .offset                            = 64,
            .size                              = 32,
        },
        {
            .stageFlags                        = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset                            = 96,
            .size                              = 32,
        },
    };

    VkPipelineLayoutCreateInfo layout_create_info   = {};
    layout_create_info.sType                        = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.pNext                        = nullptr;
    layout_create_info.flags                        = 0;
    layout_create_info.setLayoutCount               = 1;
    layout_create_info.pSetLayouts                  = &bindless_descriptor.set_layout;
    layout_create_info.pushConstantRangeCount       = sizeof(push_constants) / sizeof(VkPushConstantRange);
    layout_create_info.pPushConstantRanges          = push_constants;

    VKRESULT(vkCreatePipelineLayout(context.device, &layout_create_info, nullptr, &bindless_descriptor.pipeline_layout))


    VkDescriptorSetAllocateInfo descriptor_set_allocate_info    = {};
    descriptor_set_allocate_info.sType                          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext                          = VK_NULL_HANDLE;
    descriptor_set_allocate_info.descriptorPool                 = descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount             = 1;
    descriptor_set_allocate_info.pSetLayouts                    = &bindless_descriptor.set_layout;

    vkAllocateDescriptorSets(context.device, &descriptor_set_allocate_info, &bindless_descriptor.set);
}

vkapi::~vkapi() {
    // Free bindless descriptor stuff
    vkDestroyDescriptorSetLayout(context.device, bindless_descriptor.set_layout, nullptr);
    vkDestroyPipelineLayout(context.device, bindless_descriptor.pipeline_layout, nullptr);
    vkFreeDescriptorSets(context.device, descriptor_pool, 1, &bindless_descriptor.set);

    vkDestroyCommandPool(context.device, command_pool, nullptr);
    vkDestroyDescriptorPool(context.device, descriptor_pool, nullptr);
}


Buffer vkapi::create_buffer(size_t data_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage mem_usage) {
    Buffer buffer = {};

    VkBufferCreateInfo buffer_info  = {};
    buffer_info.sType               = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size                = data_size;
    buffer_info.usage               = buffer_usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
     
    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = mem_usage;
    alloc_create_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VKRESULT(vmaCreateBuffer(context.allocator, &buffer_info, &alloc_create_info, &buffer.handle, &buffer.alloc, &buffer.alloc_info))

    VkBufferDeviceAddressInfo buffer_device_address_info = {};
    buffer_device_address_info.sType    = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    buffer_device_address_info.pNext    = VK_NULL_HANDLE;
    buffer_device_address_info.buffer   = buffer.handle;

    buffer.size = data_size;

    buffer.device_address = vkGetBufferDeviceAddress(context.device, &buffer_device_address_info);

    return std::move(buffer);
}

void vkapi::copy_buffer(VkCommandBuffer cmd_buf, Buffer src, Buffer dst, size_t size) {
    VkBufferCopy buffer_copy = {};
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = size;

    vkCmdCopyBuffer(cmd_buf, src.handle, dst.handle, 1, &buffer_copy);
}

void vkapi::copy_buffer(VkCommandBuffer cmd_buf, Buffer src, image dst) {
    VkImageSubresourceLayers image_subresource_layers   = {};
    image_subresource_layers.aspectMask                 = dst.subresource_range.aspectMask;
    image_subresource_layers.mipLevel                   = 0;
    image_subresource_layers.baseArrayLayer             = 0;
    image_subresource_layers.layerCount                 = 1;

    VkBufferImageCopy buffer_to_image_copy  = {};
    buffer_to_image_copy.bufferOffset       = 0;
    buffer_to_image_copy.bufferRowLength    = 0;
    buffer_to_image_copy.bufferImageHeight  = 0;
    buffer_to_image_copy.imageSubresource   = image_subresource_layers;
    buffer_to_image_copy.imageOffset        = { 0, 0, 0 };
    buffer_to_image_copy.imageExtent        = dst.size;

    vkCmdCopyBufferToImage(cmd_buf, src.handle, dst.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_to_image_copy);
}

void vkapi::destroy_buffer(Buffer& buffer) {
    vmaDestroyBuffer(context.allocator, buffer.handle, buffer.alloc);
}


image vkapi::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usages) {
    image image;

    image.size = size;

    image.subresource_range.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
    image.subresource_range.baseMipLevel    = 0;
    image.subresource_range.levelCount      = 1;
    image.subresource_range.baseArrayLayer  = 0;
    image.subresource_range.layerCount      = 1;

    VkImageCreateInfo img_create_info       = {};
    img_create_info.sType                   = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_create_info.pNext                   = VK_NULL_HANDLE;
    img_create_info.flags                   = 0;
    img_create_info.imageType               = VK_IMAGE_TYPE_2D;
    img_create_info.format                  = format;
    img_create_info.extent                  = size;
    img_create_info.mipLevels               = 1;
    img_create_info.arrayLayers             = 1;
    img_create_info.samples                 = VK_SAMPLE_COUNT_1_BIT;
    img_create_info.tiling                  = VK_IMAGE_TILING_OPTIMAL;
    img_create_info.usage                   = usages;
    img_create_info.sharingMode             = VK_SHARING_MODE_EXCLUSIVE;
    img_create_info.queueFamilyIndexCount   = 0;
    img_create_info.pQueueFamilyIndices     = nullptr;
    img_create_info.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VKRESULT(vmaCreateImage(context.allocator, &img_create_info, &alloc_create_info, &image.handle, &image.alloc, &image.alloc_info))

    VkImageViewCreateInfo image_view_create_info    = {};
    image_view_create_info.sType                    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext                    = nullptr;
    image_view_create_info.flags                    = 0;
    image_view_create_info.image                    = image.handle;
    image_view_create_info.viewType                 = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                   = format;
    image_view_create_info.components               = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    };
    image_view_create_info.subresourceRange        = image.subresource_range;

    VKRESULT(vkCreateImageView(context.device, &image_view_create_info, VK_NULL_HANDLE, &image.view))

    image.bindless_storage_index = bindless_descriptor.allocate(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    image.bindless_sampled_index = bindless_descriptor.allocate(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

    return std::move(image);
}
void vkapi::destroy_image(image &image) {
    bindless_descriptor.free(image.bindless_storage_index, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    bindless_descriptor.free(image.bindless_sampled_index, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

    vmaDestroyImage(context.allocator, image.handle, image.alloc);
    vkDestroyImageView(context.device, image.view, nullptr);
}

std::vector<image> vkapi::create_images(VkExtent3D size, VkFormat format, VkImageUsageFlags usages, size_t image_count) {
    std::vector<image> images { image_count };

    for (size_t image_index = 0; image_index < image_count; image_index++) {
        images[image_index] = create_image(size, format, usages);
    }

    return std::move(images);
}

void vkapi::destroy_images(std::vector<image> &images) {
    for (auto &image: images) {
        destroy_image(image);
    }
}


VkFence vkapi::create_fence() {
    VkFence fence;
    VkFenceCreateInfo create_info   = {};
    create_info.sType               = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    create_info.pNext               = nullptr;
    create_info.flags               = VK_FENCE_CREATE_SIGNALED_BIT;

    VKRESULT(vkCreateFence(context.device, &create_info, nullptr, &fence))

    return fence;
}

void vkapi::destroy_fence(VkFence fence) {
    vkDestroyFence(context.device, fence, nullptr);
}

std::vector<VkFence> vkapi::create_fences(size_t fences_count) {
    std::vector<VkFence> fences { fences_count };

    for(size_t fence_index = 0; fence_index < fences_count; fence_index++) {
        fences[fence_index] = create_fence();
    }

    return std::move(fences);
}

void vkapi::destroy_fences(std::vector<VkFence> &fences) {
    for(auto &fence: fences) {
        vkDestroyFence(context.device, fence, nullptr);
    }
}


VkSemaphore vkapi::create_semaphore() {
    VkSemaphore semaphore;
    VkSemaphoreCreateInfo create_info   = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    create_info.pNext                   = nullptr;
    create_info.flags                   = 0;

    VKRESULT(vkCreateSemaphore(context.device, &create_info, nullptr, &semaphore))

    return semaphore;
}

void vkapi::destroy_semaphore(VkSemaphore semaphore) {
    vkDestroySemaphore(context.device, semaphore, nullptr);
}

std::vector<VkSemaphore> vkapi::create_semaphores(size_t semaphores_count) {
    std::vector<VkSemaphore> semaphores { semaphores_count };

    for (size_t semaphore_index = 0; semaphore_index < semaphores_count; semaphore_index++) {
        semaphores[semaphore_index] = create_semaphore();
    }

    return std::move(semaphores);
}

void vkapi::destroy_semaphores(std::vector<VkSemaphore> &semaphores) {
    for (auto &semaphore: semaphores) {
        vkDestroySemaphore(context.device, semaphore, nullptr);
    }
}


sampler vkapi::create_sampler(VkFilter filter, VkSamplerAddressMode address_mode) {
    VkSamplerCreateInfo sampler_create_info     = {};
    sampler_create_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext                   = VK_NULL_HANDLE;
    sampler_create_info.flags                   = 0;
    sampler_create_info.magFilter               = filter;
    sampler_create_info.minFilter               = filter;
    sampler_create_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_create_info.addressModeU            = address_mode;
    sampler_create_info.addressModeV            = address_mode;
    sampler_create_info.addressModeW            = address_mode;
    sampler_create_info.mipLodBias              = 0.f;
    sampler_create_info.anisotropyEnable        = VK_FALSE;
    sampler_create_info.compareEnable           = VK_FALSE;
    sampler_create_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.minLod                  = 0.f;
    sampler_create_info.maxLod                  = 0.f;
    sampler_create_info.borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    sampler sampler;
    VKRESULT(vkCreateSampler(context.device, &sampler_create_info, VK_NULL_HANDLE, &sampler.handle))

    sampler.bindless_index = bindless_descriptor.allocate(VK_DESCRIPTOR_TYPE_SAMPLER);

    return std::move(sampler);
}

void vkapi::destroy_sampler(sampler sampler) {
    bindless_descriptor.free(sampler.bindless_index, VK_DESCRIPTOR_TYPE_SAMPLER);

    vkDestroySampler(context.device, sampler.handle, VK_NULL_HANDLE);
}


std::vector<VkCommandBuffer> vkapi::create_command_buffers(size_t command_buffers_count) {
    VkCommandBufferAllocateInfo cmd_buf_allocate_info   = {};
    cmd_buf_allocate_info.sType                         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_allocate_info.pNext                         = VK_NULL_HANDLE;
    cmd_buf_allocate_info.commandPool                   = command_pool;
    cmd_buf_allocate_info.level                         = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_allocate_info.commandBufferCount            = command_buffers_count;

    std::vector<VkCommandBuffer> command_buffers { command_buffers_count };
    vkAllocateCommandBuffers(context.device, &cmd_buf_allocate_info, command_buffers.data());

    return std::move(command_buffers);
}

void vkapi::destroy_command_buffers(std::vector<VkCommandBuffer> &command_buffers) {
    vkFreeCommandBuffers(context.device, command_pool, command_buffers.size(), command_buffers.data());
}


// TODO: attachment structure
VkRenderPass vkapi::create_render_pass(std::vector<VkFormat>& color_attachments_format, VkImageLayout initial_layout, VkImageLayout final_layout) {
    std::vector<VkAttachmentDescription> attachments_description        = {};
    std::vector<VkAttachmentReference> attachments_reference            = {};

    for (size_t attachment_index = 0; attachment_index < color_attachments_format.size(); attachment_index++) {
        auto format = color_attachments_format[attachment_index];
        VkAttachmentDescription attachment_description                  = {};
        attachment_description.flags                                    = 0;
        attachment_description.format                                   = format;
        attachment_description.samples                                  = VK_SAMPLE_COUNT_1_BIT;
        attachment_description.loadOp                                   = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment_description.storeOp                                  = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_description.stencilLoadOp                            = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_description.stencilStoreOp                           = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_description.initialLayout                            = initial_layout;
        attachment_description.finalLayout                              = final_layout;

        attachments_description.push_back(attachment_description);

        VkAttachmentReference attachment_reference                      = {};
        attachment_reference.attachment                                 = attachment_index;
        attachment_reference.layout                                     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachments_reference.push_back(attachment_reference);
    }

    VkSubpassDescription subpass_description                            = {};
    subpass_description.flags                                           = 0;
    subpass_description.pipelineBindPoint                               = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount                            = attachments_reference.size();
    subpass_description.pColorAttachments                               = attachments_reference.data();
    subpass_description.pDepthStencilAttachment                         = VK_NULL_HANDLE;
    subpass_description.pResolveAttachments                             = VK_NULL_HANDLE;
    subpass_description.preserveAttachmentCount                         = 0;
    subpass_description.pPreserveAttachments                            = VK_NULL_HANDLE;

    VkRenderPass render_pass;
    VkRenderPassCreateInfo create_info                                  = {};
    create_info.sType                                                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.pNext                                                   = VK_NULL_HANDLE;
    create_info.flags                                                   = 0;
    create_info.attachmentCount                                         = attachments_description.size();
    create_info.pAttachments                                            = attachments_description.data();
    create_info.subpassCount                                            = 1;
    create_info.pSubpasses                                              = &subpass_description;

    VKRESULT(vkCreateRenderPass(context.device, &create_info, VK_NULL_HANDLE, &render_pass))

    return render_pass;
}

void vkapi::destroy_render_pass(VkRenderPass render_pass) {
    vkDestroyRenderPass(context.device, render_pass, VK_NULL_HANDLE);
}


VkFramebuffer vkapi::create_framebuffer(VkRenderPass render_pass, std::vector<image>& images, VkExtent2D size) {
    VkFramebuffer framebuffer;
    std::vector<VkImageView> attachements;
    for (auto& image: images) {
        attachements.push_back(image.view);
    }

    VkFramebufferCreateInfo create_info = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.pNext                   = VK_NULL_HANDLE;
    create_info.flags                   = 0;
    create_info.renderPass              = render_pass;
    create_info.attachmentCount         = attachements.size();
    create_info.pAttachments            = attachements.data();
    create_info.width                   = size.width;
    create_info.height                  = size.height;
    create_info.layers                  = 1;

    VKRESULT(vkCreateFramebuffer(context.device, &create_info, VK_NULL_HANDLE, &framebuffer))

    return framebuffer;
}

std::vector<VkFramebuffer> vkapi::create_framebuffers(VkRenderPass render_pass, std::vector<image>& images, VkExtent2D size, uint32_t framebuffer_count) {
    std::vector<VkFramebuffer> framebuffers { framebuffer_count };

    for (auto& framebuffer: framebuffers) {
        framebuffer = create_framebuffer(render_pass, images, size);
    }

    return std::move(framebuffers);
}

void vkapi::destroy_framebuffer(VkFramebuffer framebuffer) {
    vkDestroyFramebuffer(context.device, framebuffer, VK_NULL_HANDLE);
}

void vkapi::destroy_framebuffers(std::vector<VkFramebuffer>& framebuffers) {
    for (auto& framebuffer: framebuffers) {
        vkDestroyFramebuffer(context.device, framebuffer, VK_NULL_HANDLE);
    }
}


Pipeline vkapi::create_compute_pipeline(const char* shader_name) {
    Pipeline pipeline {
        .bind_point = VK_PIPELINE_BIND_POINT_COMPUTE
    };

    char shader_path[256] = {};
    sprintf(shader_path, "%s%s%s", "./shaders/", shader_name, ".comp.spv");
    std::vector<uint8_t> shader_code = read_file(shader_path);
    if (shader_code.size() == 0) {
        exit(1);
    }

    pipeline.shader_modules.resize(1);

    VkShaderModuleCreateInfo shader_create_info     = {};
    shader_create_info.sType                        = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext                        = VK_NULL_HANDLE;
    shader_create_info.flags                        = 0;
    shader_create_info.codeSize                     = shader_code.size();
    shader_create_info.pCode                        = (uint32_t*)shader_code.data();

    VKRESULT(vkCreateShaderModule(context.device, &shader_create_info, VK_NULL_HANDLE, &pipeline.shader_modules[0]))

    VkPipelineShaderStageCreateInfo stage_create_info = {};
    stage_create_info.sType                         = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.pNext                         = VK_NULL_HANDLE;
    stage_create_info.flags                         = 0;
    stage_create_info.stage                         = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_create_info.module                        = pipeline.shader_modules[0];
    stage_create_info.pName                         = "main";
    stage_create_info.pSpecializationInfo           = VK_NULL_HANDLE;


    VkComputePipelineCreateInfo create_info         = {};
    create_info.sType                               = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    create_info.pNext                               = VK_NULL_HANDLE;
    create_info.flags                               = 0;
    create_info.stage                               = stage_create_info;
    create_info.layout                              = bindless_descriptor.pipeline_layout;

    VKRESULT(vkCreateComputePipelines(
        context.device,
        VK_NULL_HANDLE,
        1,
        &create_info,
        nullptr,
        &pipeline.handle
    ))

    return std::move(pipeline);
}

Pipeline vkapi::create_graphics_pipeline(const char* shader_name, VkShaderStageFlagBits shader_stages, VkRenderPass render_pass, std::vector<VkDynamicState> dynamic_states) {
    Pipeline pipeline {
        .bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS
    };

    std::vector<VkPipelineShaderStageCreateInfo> stages_create_info = {};

    for (VkShaderStageFlags stage_flag = VK_SHADER_STAGE_VERTEX_BIT; stage_flag <= VK_SHADER_STAGE_ALL_GRAPHICS; stage_flag <<= 2) {
        if (stage_flag & shader_stages) {
            VkShaderModule shader_module;

            char shader_path[256] = {};
            sprintf(shader_path, "%s%s%s%s", "./shaders/", shader_name, shader_stage_extension(stage_flag), ".spv");
            std::vector<uint8_t> shader_code = read_file(shader_path);
            if (shader_code.size() == 0) {
                exit(1);
            }

            VkShaderModuleCreateInfo shader_create_info     = {};
            shader_create_info.sType                        = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shader_create_info.pNext                        = nullptr;
            shader_create_info.flags                        = 0;
            shader_create_info.codeSize                     = shader_code.size();
            shader_create_info.pCode                        = (uint32_t*)shader_code.data();

            VKRESULT(vkCreateShaderModule(context.device, &shader_create_info, nullptr, &shader_module))

            pipeline.shader_modules.push_back(shader_module);

            VkPipelineShaderStageCreateInfo stage_create_info = {};
            stage_create_info.sType                         = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage_create_info.pNext                         = nullptr;
            stage_create_info.flags                         = 0;
            stage_create_info.stage                         = (VkShaderStageFlagBits)stage_flag;
            stage_create_info.module                        = shader_module;
            stage_create_info.pName                         = "main";
            stage_create_info.pSpecializationInfo           = nullptr;

            stages_create_info.push_back(stage_create_info);
        }
    }


    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info     = {};
    vertex_input_state_create_info.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.pNext                                  = nullptr;
    input_assembly_state_create_info.flags                                  = 0;
    input_assembly_state_create_info.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_state_create_info            = {};
    viewport_state_create_info.sType                                        = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.pNext                                        = nullptr;
    viewport_state_create_info.flags                                        = 0;
    viewport_state_create_info.viewportCount                                = 1;
    viewport_state_create_info.pViewports                                   = VK_NULL_HANDLE;
    viewport_state_create_info.scissorCount                                 = 1;
    viewport_state_create_info.pScissors                                    = VK_NULL_HANDLE;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info  = {};
    rasterization_state_create_info.sType                                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.pNext                                   = nullptr;
    rasterization_state_create_info.flags                                   = 0;
    rasterization_state_create_info.depthClampEnable                        = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable                 = VK_FALSE;
    rasterization_state_create_info.polygonMode                             = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode                                = VK_CULL_MODE_NONE;
    rasterization_state_create_info.frontFace                               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable                         = VK_FALSE;
    rasterization_state_create_info.lineWidth                               = 1.f;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info      = {};
    multisample_state_create_info.sType                                     = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.pNext                                     = nullptr;
    multisample_state_create_info.flags                                     = 0;
    multisample_state_create_info.rasterizationSamples                      = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.sampleShadingEnable                       = VK_FALSE;
    multisample_state_create_info.minSampleShading                          = 1.f;

    VkPipelineColorBlendAttachmentState color_attachment_state              = {};
    color_attachment_state.blendEnable                                      = VK_TRUE;
    color_attachment_state.srcColorBlendFactor                              = VK_BLEND_FACTOR_SRC_ALPHA;
    color_attachment_state.dstColorBlendFactor                              = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment_state.colorBlendOp                                     = VK_BLEND_OP_ADD;
    color_attachment_state.srcAlphaBlendFactor                              = VK_BLEND_FACTOR_ONE;
    color_attachment_state.dstAlphaBlendFactor                              = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment_state.alphaBlendOp                                     = VK_BLEND_OP_ADD;
    color_attachment_state.colorWriteMask                                   = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info       = {};
    color_blend_state_create_info.sType                                     = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.pNext                                     = nullptr;
    color_blend_state_create_info.flags                                     = 0;
    color_blend_state_create_info.logicOpEnable                             = VK_FALSE;
    color_blend_state_create_info.attachmentCount                           = 1;
    color_blend_state_create_info.pAttachments                              = &color_attachment_state;

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info              = {};
    dynamic_state_create_info.sType                                         = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.pNext                                         = VK_NULL_HANDLE;
    dynamic_state_create_info.flags                                         = 0;
    dynamic_state_create_info.dynamicStateCount                             = dynamic_states.size();
    dynamic_state_create_info.pDynamicStates                                = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_create_info                       = {};
    pipeline_create_info.sType                                              = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext                                              = nullptr;
    pipeline_create_info.flags                                              = 0;
    pipeline_create_info.stageCount                                         = stages_create_info.size();
    pipeline_create_info.pStages                                            = stages_create_info.data();
    pipeline_create_info.pVertexInputState                                  = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState                                = &input_assembly_state_create_info;
    pipeline_create_info.pViewportState                                     = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState                                = &rasterization_state_create_info;
    pipeline_create_info.pMultisampleState                                  = &multisample_state_create_info;
    pipeline_create_info.pColorBlendState                                   = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState                                      = &dynamic_state_create_info;
    pipeline_create_info.layout                                             = bindless_descriptor.pipeline_layout;
    pipeline_create_info.renderPass                                         = render_pass;
    pipeline_create_info.subpass                                            = 0;

    VKRESULT(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipeline_create_info, VK_NULL_HANDLE, &pipeline.handle))

    return pipeline;
}

void vkapi::destroy_pipeline(Pipeline &pipeline) {
    for (auto& shader_module: pipeline.shader_modules) {
        vkDestroyShaderModule(context.device, shader_module, nullptr);
    }

    vkDestroyPipeline(context.device, pipeline.handle, nullptr);
}


std::vector<VkDescriptorSet> vkapi::create_descriptor_sets(VkDescriptorSetLayout descriptor_sets_layout, size_t descriptor_sets_count) {
    std::vector<VkDescriptorSetLayout> sets_layouts = { descriptor_sets_count, descriptor_sets_layout };

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info    = {};
    descriptor_set_allocate_info.sType                          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext                          = nullptr;
    descriptor_set_allocate_info.descriptorPool                 = descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount             = sets_layouts.size();
    descriptor_set_allocate_info.pSetLayouts                    = sets_layouts.data();

    std::vector<VkDescriptorSet> descriptor_sets { descriptor_sets_count };
    vkAllocateDescriptorSets(context.device, &descriptor_set_allocate_info, descriptor_sets.data());

    return std::move(descriptor_sets);
}

void vkapi::destroy_descriptor_sets(std::vector<VkDescriptorSet> &descriptor_sets) {
    vkFreeDescriptorSets(context.device, descriptor_pool, descriptor_sets.size(), descriptor_sets.data());
}


VkSurfaceKHR vkapi::create_surface(window& wnd) {
    VkSurfaceKHR surface;

#if defined(LINUX)
    VkXcbSurfaceCreateInfoKHR create_info   = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    create_info.connection                  = wnd.connection;
    create_info.window                      = wnd.win;

    VKRESULT(vkCreateXcbSurfaceKHR(context.instance, &create_info, nullptr, &surface))
#elif defined(WINDOWS)
    VkWin32SurfaceCreateInfoKHR create_info   = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    create_info.hinstance                   = GetModuleHandle(NULL);
    create_info.hwnd                        = wnd.win_handle;

    VKRESULT(vkCreateWin32SurfaceKHR(context.instance, &create_info, nullptr, &surface))
#elif defined(MACOS)
    VkMetalSurfaceCreateInfoEXT create_info = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    // create_info.pLayer                   =   GetModuleHandle(NULL);

    VKRESULT(vkCreateMetalSurfaceEXT(context.instance, &create_info, nullptr, &surface))
#endif

    return surface;
}

void vkapi::destroy_surface(VkSurfaceKHR surface) {
    vkDestroySurfaceKHR(context.instance, surface, nullptr);
}


Swapchain vkapi::create_swapchain(VkSurfaceKHR surface, size_t min_image_count, VkImageUsageFlags usages, VkSwapchainKHR old_swapchain) {
    Swapchain swapchain;
    VkBool32 queue_support_presentation = VK_FALSE;
    VKRESULT(vkGetPhysicalDeviceSurfaceSupportKHR(context.physical_device, context.queue_index, surface, &queue_support_presentation))

    assert(queue_support_presentation == true);

    VkSurfaceCapabilitiesKHR caps;
    VKRESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physical_device, surface, &caps))

    swapchain.extent = caps.currentExtent;

    if (swapchain.extent.width < caps.minImageExtent.width) {
        swapchain.extent.width = caps.minImageExtent.width;
    } else if (swapchain.extent.width > caps.maxImageExtent.width) {
        swapchain.extent.width = caps.maxImageExtent.width;
    }

    if (swapchain.extent.height < caps.minImageExtent.height) {
        swapchain.extent.height = caps.minImageExtent.height;
    } else if (swapchain.extent.height > caps.maxImageExtent.height) {
        swapchain.extent.height = caps.maxImageExtent.height;
    }

    uint32_t supported_formats_count = 0;
    VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physical_device, surface, &supported_formats_count,  VK_NULL_HANDLE))

    std::vector<VkSurfaceFormatKHR> supported_formats { supported_formats_count };
    VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physical_device, surface, &supported_formats_count,  supported_formats.data()))

    uint32_t supported_present_modes_count = 0;
    VKRESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physical_device, surface, &supported_present_modes_count, VK_NULL_HANDLE))

    std::vector<VkPresentModeKHR> supported_present_modes { supported_present_modes_count };
    VKRESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physical_device, surface, &supported_present_modes_count, supported_present_modes.data()))

    swapchain.surface_format = supported_formats[0];

    VkSwapchainCreateInfoKHR create_info    = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    create_info.surface                     = surface;
    create_info.minImageCount               = min_image_count;
    create_info.imageFormat                 = swapchain.surface_format.format;
    create_info.imageColorSpace             = swapchain.surface_format.colorSpace;
    create_info.imageExtent                 = swapchain.extent;
    create_info.imageArrayLayers            = 1;
    create_info.imageUsage                  = usages;
    create_info.imageSharingMode            = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform                = caps.currentTransform;
    create_info.compositeAlpha              = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // create_info.presentMode                 = VK_PRESENT_MODE_FIFO_KHR;
    create_info.presentMode                 = VK_PRESENT_MODE_MAILBOX_KHR;
    create_info.clipped                     = VK_TRUE;
    create_info.oldSwapchain                = old_swapchain;

    VKRESULT(vkCreateSwapchainKHR(context.device, &create_info, nullptr, &swapchain.handle))

    uint32_t image_count = 0;
    VKRESULT(vkGetSwapchainImagesKHR(context.device, swapchain.handle, &image_count, VK_NULL_HANDLE))

    swapchain.image_count = image_count;
    swapchain.images = std::vector<image>{ swapchain.image_count };
    auto images = std::vector<VkImage>{ swapchain.image_count };
    auto views = std::vector<VkImageView>{ swapchain.image_count };

    VKRESULT(vkGetSwapchainImagesKHR(context.device, swapchain.handle, (uint32_t*)&swapchain.image_count, images.data()))

    VkImageSubresourceRange image_subresource_range = {};
    image_subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_range.baseMipLevel            = 0;
    image_subresource_range.levelCount              = 1;
    image_subresource_range.baseArrayLayer          = 0;
    image_subresource_range.layerCount              = 1;

    VkExtent3D image_size = {
        .width   = swapchain.extent.width,
        .height  = swapchain.extent.height,
        .depth   = 1,
    };

    for (size_t image_index = 0; image_index < swapchain.image_count; image_index++) {
        swapchain.images[image_index].bindless_storage_index = bindless_descriptor.allocate(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        swapchain.images[image_index].bindless_sampled_index = bindless_descriptor.allocate(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

        swapchain.images[image_index].handle = images[image_index];
        swapchain.images[image_index].size = image_size;
        swapchain.images[image_index].subresource_range = image_subresource_range;

        VkImageViewCreateInfo image_view_create_info    = {};
        image_view_create_info.sType                    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext                    = nullptr;
        image_view_create_info.flags                    = 0;
        image_view_create_info.image                    = swapchain.images[image_index].handle;
        image_view_create_info.viewType                 = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format                   = swapchain.surface_format.format;
        image_view_create_info.components               = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        image_view_create_info.subresourceRange        = image_subresource_range;

        VKRESULT(vkCreateImageView(context.device, &image_view_create_info, nullptr, &swapchain.images[image_index].view))
    }

    return std::move(swapchain);
}

void vkapi::destroy_swapchain(Swapchain& swapchain) {
    for(auto &image: swapchain.images) {
        bindless_descriptor.free(image.bindless_storage_index, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        bindless_descriptor.free(image.bindless_sampled_index, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

        vkDestroyImageView(context.device, image.view, nullptr);
    }
    vkDestroySwapchainKHR(context.device, swapchain.handle, nullptr);
}


void vkapi::update_descriptor_images(std::vector<image> &images, VkDescriptorType type) {
    std::vector<VkDescriptorImageInfo> descriptor_images_info   { images.size() };
    std::vector<VkWriteDescriptorSet> writes_descriptor         { images.size() };

    for (size_t image_index = 0; image_index < images.size(); image_index++) {
        auto &image = images[image_index];
        descriptor_images_info[image_index].sampler         = VK_NULL_HANDLE;
        descriptor_images_info[image_index].imageView       = image.view;
        descriptor_images_info[image_index].imageLayout     = VK_IMAGE_LAYOUT_GENERAL;

        writes_descriptor[image_index].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes_descriptor[image_index].pNext                = nullptr;
        writes_descriptor[image_index].dstSet               = bindless_descriptor.set;
        writes_descriptor[image_index].dstBinding           = type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ? 2 : 1;
        writes_descriptor[image_index].dstArrayElement      = type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ? image.bindless_storage_index : image.bindless_sampled_index;
        writes_descriptor[image_index].descriptorCount      = 1;
        writes_descriptor[image_index].descriptorType       = type;
        writes_descriptor[image_index].pImageInfo           = &descriptor_images_info[image_index];
        writes_descriptor[image_index].pBufferInfo          = VK_NULL_HANDLE;
        writes_descriptor[image_index].pTexelBufferView     = VK_NULL_HANDLE;
    }

    vkUpdateDescriptorSets(context.device, writes_descriptor.size(), writes_descriptor.data(), 0, nullptr);
}

void vkapi::update_descriptor_samplers(std::vector<sampler> &samplers) {
    std::vector<VkDescriptorImageInfo> descriptor_images_info   { samplers.size() };
    std::vector<VkWriteDescriptorSet> writes_descriptor         { samplers.size() };

    for (size_t sampler_index = 0; sampler_index < samplers.size(); sampler_index++) {
        descriptor_images_info[sampler_index].sampler         = samplers[sampler_index].handle;
        descriptor_images_info[sampler_index].imageView       = VK_NULL_HANDLE;
        descriptor_images_info[sampler_index].imageLayout     = VK_IMAGE_LAYOUT_GENERAL;

        writes_descriptor[sampler_index].sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes_descriptor[sampler_index].pNext                = nullptr;
        writes_descriptor[sampler_index].dstSet               = bindless_descriptor.set;
        writes_descriptor[sampler_index].dstBinding           = 0;
        writes_descriptor[sampler_index].dstArrayElement      = samplers[sampler_index].bindless_index;
        writes_descriptor[sampler_index].descriptorCount      = 1;
        writes_descriptor[sampler_index].descriptorType       = VK_DESCRIPTOR_TYPE_SAMPLER;
        writes_descriptor[sampler_index].pImageInfo           = &descriptor_images_info[sampler_index];
        writes_descriptor[sampler_index].pBufferInfo          = VK_NULL_HANDLE;
        writes_descriptor[sampler_index].pTexelBufferView     = VK_NULL_HANDLE;
    }

    vkUpdateDescriptorSets(context.device, writes_descriptor.size(), writes_descriptor.data(), 0, nullptr);
}


void vkapi::start_record(VkCommandBuffer command_buffer) {
    VkCommandBufferBeginInfo cmd_buf_begin_info = {};
    cmd_buf_begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_begin_info.pNext                    = nullptr;
    cmd_buf_begin_info.flags                    = 0;
    cmd_buf_begin_info.pInheritanceInfo         = nullptr;

    vkBeginCommandBuffer(command_buffer, &cmd_buf_begin_info);
}

void vkapi::image_barrier(VkCommandBuffer command_buffer, VkImageLayout src_layout, VkImageLayout dst_layout, VkPipelineStageFlagBits src_stage, VkPipelineStageFlagBits dst_stage, VkAccessFlags src_access, VkAccessFlags dst_access, image image) {
    VkImageMemoryBarrier undefined_to_general_barrier   = {};
    undefined_to_general_barrier.sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    undefined_to_general_barrier.pNext                  = nullptr;
    undefined_to_general_barrier.srcAccessMask          = src_access;
    undefined_to_general_barrier.dstAccessMask          = dst_access;
    undefined_to_general_barrier.oldLayout              = src_layout;
    undefined_to_general_barrier.newLayout              = dst_layout;
    undefined_to_general_barrier.srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
    undefined_to_general_barrier.dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
    undefined_to_general_barrier.image                  = image.handle;
    undefined_to_general_barrier.subresourceRange       = image.subresource_range;

    vkCmdPipelineBarrier(
        command_buffer,
        src_stage,
        dst_stage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &undefined_to_general_barrier
    );
}


void vkapi::begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D size, Pipeline pipeline) {
    VkRect2D render_area = {
        {
            0,
            0,
        },
        size
    };

    VkClearValue clear_value = { 0.f, 0.f, 0.f, 1.f };
    VkRenderPassBeginInfo render_pass_begin_info    = {};
    render_pass_begin_info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext                    = VK_NULL_HANDLE;
    render_pass_begin_info.renderPass               = render_pass;
    render_pass_begin_info.framebuffer              = framebuffer;
    render_pass_begin_info.renderArea               = render_area;
    render_pass_begin_info.clearValueCount          = 1;
    render_pass_begin_info.pClearValues             = &clear_value;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, pipeline.bind_point, pipeline.handle);
}

void vkapi::end_render_pass(VkCommandBuffer command_buffer) {
    vkCmdEndRenderPass(command_buffer);
}


void vkapi::run_compute_pipeline(VkCommandBuffer command_buffer, Pipeline pipeline, size_t group_count_x, size_t group_count_y, size_t group_count_z) {
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, bindless_descriptor.pipeline_layout, 0, 1, &bindless_descriptor.set, 0, nullptr);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.handle);

    vkCmdDispatch(command_buffer, group_count_x, group_count_y, group_count_z);
}

void vkapi::draw(VkCommandBuffer command_buffer, Pipeline pipeline, uint32_t vertex_count, uint32_t vertex_offset) {
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bindless_descriptor.pipeline_layout, 0, 1, &bindless_descriptor.set, 0, nullptr);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);

    vkCmdDraw(command_buffer, vertex_count, 1, vertex_offset, 0);
}

void vkapi::draw(VkCommandBuffer command_buffer, Pipeline pipeline, Buffer index_buffer, uint32_t index_count, uint32_t index_offset, uint32_t vertex_offset) {
    vkCmdBindIndexBuffer(command_buffer, index_buffer.handle, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bindless_descriptor.pipeline_layout, 0, 1, &bindless_descriptor.set, 0, nullptr);

    vkCmdDrawIndexed(command_buffer, index_count, 1, index_offset, vertex_offset, 0);
}

void vkapi::blit_full(VkCommandBuffer command_buffer, image src_image, image dst_image) {
    VkImageSubresourceLayers images_subres_layers {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel       = 0,
        .baseArrayLayer = 0,
        .layerCount     = 1,
    };

    VkOffset3D start_offset {
        .x  = 0,
        .y  = 0,
        .z  = 0,
    };
    VkOffset3D end_offset {
        .x  = (int32_t)src_image.size.width,
        .y  = (int32_t)src_image.size.height,
        .z  = (int32_t)src_image.size.depth,
    };

    VkImageBlit blit_region {
        .srcSubresource = images_subres_layers,
        .srcOffsets     = { start_offset, end_offset },
        .dstSubresource = images_subres_layers,
        .dstOffsets     = { start_offset, end_offset },
    };

    vkCmdBlitImage(command_buffer, src_image.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region, VK_FILTER_NEAREST);
}

void vkapi::end_record(VkCommandBuffer command_buffer) {
    VKRESULT(vkEndCommandBuffer(command_buffer))
}

VkResult vkapi::submit(VkCommandBuffer command_buffer, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, VkFence submission_fence) {
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    VkSubmitInfo submit_info            = {};
    submit_info.sType                   = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                   = nullptr;
    submit_info.waitSemaphoreCount      = wait_semaphore ? 1 : 0;
    submit_info.pWaitSemaphores         = &wait_semaphore;
    submit_info.pWaitDstStageMask       = wait_semaphore ? &wait_stage : VK_NULL_HANDLE;
    submit_info.signalSemaphoreCount    = signal_semaphore ? 1 : 0;
    submit_info.pSignalSemaphores       = &signal_semaphore;
    submit_info.commandBufferCount      = 1;
    submit_info.pCommandBuffers         = &command_buffer;

    return vkQueueSubmit(context.queue, 1, &submit_info, submission_fence);
}

VkResult vkapi::present(Swapchain& swapchain, uint32_t image_index, VkSemaphore wait_semaphore) {
    VkPresentInfoKHR present_info   = {};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext              = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &wait_semaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain.handle;
    present_info.pImageIndices      = &image_index;
    present_info.pResults           = nullptr;

    return vkQueuePresentKHR(context.queue, &present_info);
}


const char* vkapi::shader_stage_extension(VkShaderStageFlags shader_stage) {
    switch(shader_stage) {
        case VK_SHADER_STAGE_VERTEX_BIT: {
            return ".vert";
        }
        case VK_SHADER_STAGE_FRAGMENT_BIT: {
            return ".frag";
        }
        case VK_SHADER_STAGE_COMPUTE_BIT: {
            return ".comp";
        }
        default:
            std::cerr << "Stage not supported" << std::endl;
            return "";
    }
}
