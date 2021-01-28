#include <cassert>
#include <memory>

#include "vk-api.hpp"
#include "utils.hpp"

#define VKRESULT(result) assert(result == VK_SUCCESS);


vkapi::vkapi() {
    VkCommandPoolCreateInfo cmd_pool_create_info    = {};
    cmd_pool_create_info.sType                      = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.pNext                      = nullptr;
    cmd_pool_create_info.flags                      = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_create_info.queueFamilyIndex           = context.queue_index;

    VKRESULT(vkCreateCommandPool(context.device, &cmd_pool_create_info, nullptr, &command_pool))


    std::vector<VkDescriptorPoolSize> descriptor_pools_sizes = {
        {
            .type                               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount                    = (uint32_t)0xff,
        },
        {
            .type                               = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount                    = (uint32_t)0xff,
        }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info  = {};
    descriptor_pool_create_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext                       = nullptr;
    descriptor_pool_create_info.flags                       = 0;
    descriptor_pool_create_info.maxSets                     = 0xff;
    descriptor_pool_create_info.poolSizeCount               = descriptor_pools_sizes.size();
    descriptor_pool_create_info.pPoolSizes                  = descriptor_pools_sizes.data();

    VKRESULT(vkCreateDescriptorPool(context.device, &descriptor_pool_create_info, nullptr, &descriptor_pool))
}

vkapi::~vkapi() {
    vkDestroyCommandPool(context.device, command_pool, nullptr);
    vkDestroyDescriptorPool(context.device, descriptor_pool, nullptr);
}


Buffer vkapi::create_buffer(size_t data_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage mem_usage) {
    Buffer buffer = {};

    VkBufferCreateInfo buffer_info  = {  };
    buffer_info.sType               = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size                = data_size;
    buffer_info.usage               = buffer_usage;
     
    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = mem_usage;
     
    vmaCreateBuffer(context.allocator, &buffer_info, &alloc_create_info, &buffer.vkbuffer, &buffer.alloc, nullptr);

    vmaMapMemory(context.allocator, buffer.alloc, (void**) &buffer.mapped_ptr);

    return std::move(buffer);
}

void vkapi::destroy_buffer(Buffer& buffer) {
    vmaUnmapMemory(context.allocator, buffer.alloc);
    vmaDestroyBuffer(context.allocator, buffer.vkbuffer, buffer.alloc);
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

    for(auto &fence: fences) {
        VkFenceCreateInfo create_info   = {};
        create_info.sType               = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        create_info.pNext               = nullptr;
        create_info.flags               = VK_FENCE_CREATE_SIGNALED_BIT;

        VKRESULT(vkCreateFence(context.device, &create_info, nullptr, &fence))
    }

    return std::move(fences);
}

void vkapi::destroy_fences(std::vector<VkFence> fences) {
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

    for (auto &semaphore: semaphores) {
        VkSemaphoreCreateInfo create_info   = {};
        create_info.sType                   = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        create_info.pNext                   = nullptr;
        create_info.flags                   = 0;

        VKRESULT(vkCreateSemaphore(context.device, &create_info, nullptr, &semaphore))
    }

    return std::move(semaphores);
}

void vkapi::destroy_semaphores(std::vector<VkSemaphore> &semaphores) {
    for (auto &semaphore: semaphores) {
        vkDestroySemaphore(context.device, semaphore, nullptr);
    }
}


std::vector<VkCommandBuffer> vkapi::create_command_buffers(size_t command_buffers_count) {
    VkCommandBufferAllocateInfo cmd_buf_allocate_info   = {};
    cmd_buf_allocate_info.sType                         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_allocate_info.pNext                         = nullptr;
    cmd_buf_allocate_info.commandPool                   = command_pool;
    cmd_buf_allocate_info.level                         = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_allocate_info.commandBufferCount            = command_buffers_count;

    std::vector<VkCommandBuffer> command_buffers { command_buffers_count };
    vkAllocateCommandBuffers(context.device, &cmd_buf_allocate_info, command_buffers.data());

    return std::move(command_buffers);
}

void vkapi::destroy_command_buffers(std::vector<VkCommandBuffer> command_buffers) {
    vkFreeCommandBuffers(context.device, command_pool, command_buffers.size(), command_buffers.data());
}


ComputePipeline vkapi::create_compute_pipeline(const char* shader_path, std::vector<VkDescriptorSetLayoutBinding> bindings) {
    ComputePipeline pipeline;

    std::vector<uint8_t> shader_code = get_shader_code(shader_path);
    if (shader_code.size() == 0) {
        exit(1);
    }

    VkShaderModuleCreateInfo shader_create_info     = {};
    shader_create_info.sType                        = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext                        = nullptr;
    shader_create_info.flags                        = 0;
    shader_create_info.codeSize                     = shader_code.size();
    shader_create_info.pCode                        = (uint32_t*)shader_code.data();

    VKRESULT(vkCreateShaderModule(context.device, &shader_create_info, nullptr, &pipeline.shader_module))

    VkPipelineShaderStageCreateInfo stage_create_info = {};
    stage_create_info.sType                         = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.pNext                         = nullptr;
    stage_create_info.flags                         = 0;
    stage_create_info.stage                         = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_create_info.module                        = pipeline.shader_module;
    stage_create_info.pName                         = "main";
    stage_create_info.pSpecializationInfo           = nullptr;


    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext         = nullptr;
    descriptor_set_layout_create_info.flags         = 0;
    descriptor_set_layout_create_info.bindingCount  = bindings.size();
    descriptor_set_layout_create_info.pBindings     = bindings.data();

    VKRESULT(vkCreateDescriptorSetLayout(context.device, &descriptor_set_layout_create_info, nullptr, &pipeline.descriptor_set_layout))

    VkPipelineLayoutCreateInfo layout_create_info   = {};
    layout_create_info.sType                        = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.pNext                        = nullptr;
    layout_create_info.flags                        = 0;
    layout_create_info.setLayoutCount               = 1;
    layout_create_info.pSetLayouts                  = &pipeline.descriptor_set_layout;
    layout_create_info.pushConstantRangeCount       = 0;
    layout_create_info.pPushConstantRanges          = nullptr;

    VKRESULT(vkCreatePipelineLayout(context.device, &layout_create_info, nullptr, &pipeline.layout))

    VkComputePipelineCreateInfo create_info         = {};
    create_info.sType                               = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    create_info.pNext                               = nullptr;
    create_info.flags                               = 0;
    create_info.stage                               = stage_create_info;
    create_info.layout                              = pipeline.layout;

    VKRESULT(vkCreateComputePipelines(
        context.device,
        VK_NULL_HANDLE,
        1,
        &create_info,
        nullptr,
        &pipeline.vkpipeline
    ))

    return std::move(pipeline);
}

void vkapi::destroy_compute_pipeline(ComputePipeline &pipeline) {
    vkDestroyShaderModule(context.device, pipeline.shader_module, nullptr);
    vkDestroyDescriptorSetLayout(context.device, pipeline.descriptor_set_layout, nullptr);
    vkDestroyPipelineLayout(context.device, pipeline.layout, nullptr);
    vkDestroyPipeline(context.device, pipeline.vkpipeline, nullptr);
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

void vkapi::destroy_descriptor_sets(std::vector<VkDescriptorSet> descriptor_sets) {
    vkFreeDescriptorSets(context.device, descriptor_pool, descriptor_sets.size(), descriptor_sets.data());
}
