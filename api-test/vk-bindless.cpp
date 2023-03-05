#include "vk-bindless.hpp"

#include <vk_mem_alloc.h>

#include "vk-device.hpp"
#include "vk-utils.hpp"
#include "vulkan-loader.hpp"

const device_buffer& bindless_model::get_uniform_buffer() const {
    auto& render_device = vkdevice::get_render_device();
    return render_device.get_buffer(draws_uniform_buffer_handle);
}

bindless_model bindless_model::create_bindless_model() {
    bindless_model bindless;
    auto& render_device = vkdevice::get_render_device();
    auto* device = render_device.get_device();
#ifndef USE_VK_DESCRIPTOR_BUFFER
    bindless.create_immutable_samplers(device);
    bindless.create_layout(device);
    bindless.create_descriptor_pool(device);
    bindless.allocate_sets(device);

    VkWriteDescriptorSet writes_descriptor_set[2U];

    {
        bindless.instances_uniform_buffer_handle = render_device.create_buffer({
            .size = instances_uniform_buffer_size,
            .usages = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        });

        const auto& buffer = render_device.get_buffer(bindless.instances_uniform_buffer_handle);
        VkDescriptorBufferInfo buffer_info {
            .buffer = buffer.vk_buffer,
            .offset = 0U,
            .range = VK_WHOLE_SIZE,
        };

        writes_descriptor_set[0U] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = bindless.sets[BindlessSetType::INSTANCES_UNIFORMS],
            .dstBinding = 0U,
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .pImageInfo = nullptr,
            .pBufferInfo = &buffer_info,
            .pTexelBufferView = nullptr,
        };
    }

    {
        bindless.draws_uniform_buffer_handle = render_device.create_buffer({
            .size = draws_uniform_buffer_size,
            .usages = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        });

        const auto& buffer = render_device.get_buffer(bindless.draws_uniform_buffer_handle);
        VkDescriptorBufferInfo buffer_info {
            .buffer = buffer.vk_buffer,
            .offset = 0U,
            .range = VK_WHOLE_SIZE,
        };

        writes_descriptor_set[1U] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = bindless.sets[BindlessSetType::DRAWS_UNIFORMS],
            .dstBinding = 0U,
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .pImageInfo = nullptr,
            .pBufferInfo = &buffer_info,
            .pTexelBufferView = nullptr,
        };
    }

    vkUpdateDescriptorSets(device, sizeof(writes_descriptor_set) / sizeof(VkWriteDescriptorSet), writes_descriptor_set, 0U, nullptr);
#else
    VkBufferCreateInfo create_info{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0U,
        .size                  = desc.size,
        .usage                 = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
    };

    VmaAllocatorCreateFlags alloc_create_flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationCreateInfo alloc_create_info{
        .flags          = alloc_create_flags,
        .usage          = VMA_MEMORY_USAGE_CPU_TO_GPU,
        .requiredFlags  = desc.memory_properties,
        .preferredFlags = 0U,
        .memoryTypeBits = 0U,
        .pool           = nullptr,
        .pUserData      = nullptr,
    };

    VmaAllocationInfo alloc_info{};

    VKCHECK(vmaCreateBuffer(gpu_allocator, &create_info, &alloc_create_info, &device_buffer.vk_buffer, &device_buffer.alloc, &alloc_info));

    device_buffer.mapped_ptr = alloc_info.pMappedData;

    return { buffers.add(device_buffer) };
#endif

    return bindless;
}

void bindless_model::destroy_bindless_model(bindless_model& bindless) {
    auto& render_device = vkdevice::get_render_device();
    auto* device = vkdevice::get_render_device().get_device();
    vkDestroyDescriptorPool(device, bindless.descriptor_pool, nullptr);

    for (auto index{ 0U }; index < BindlessSetType::MAX; index++) {
        vkDestroyDescriptorSetLayout(device, bindless.sets_layout[index], nullptr);
    }

    vkDestroyPipelineLayout(device, bindless.layout, nullptr);

    bindless.destroy_immutable_samplers(device);

    render_device.destroy_buffer(bindless.instances_uniform_buffer_handle);
    render_device.destroy_buffer(bindless.draws_uniform_buffer_handle);
}

void bindless_model::create_layout(VkDevice device) {
    // Set 0 global resources
    //  Binding 0 Storage images
    //  Binding 1 Sampled images
    //  Binding 2 Samplers
    {
        VkDescriptorSetLayoutBinding set_layout_bindings[]{
            {
                .binding            = 0U,
                .descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLER,
                .descriptorCount    = SamplerType::MAX_SAMPLER,
                .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = samplers,
            },
            {
                .binding            = 1U,
                .descriptorType     = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount    = set_descriptors_count[BindlessSetType::GLOBAL],
                .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
            {
                .binding            = 2U,
                .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount    = set_descriptors_count[BindlessSetType::GLOBAL],
                .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
        };

        VkDescriptorBindingFlags flags[]{
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
        };

        VkDescriptorSetLayoutBindingFlagsCreateInfo layout_binding_flags{
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .pNext         = VK_NULL_HANDLE,
            .bindingCount  = sizeof(flags) / sizeof(VkDescriptorBindingFlags),
            .pBindingFlags = flags,
        };

        VkDescriptorSetLayoutCreateInfo create_info{
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext        = &layout_binding_flags,
            .flags        = 0U,
            .bindingCount = sizeof(set_layout_bindings) / sizeof(VkDescriptorSetLayoutBinding),
            .pBindings    = set_layout_bindings,
        };

        VKCHECK(vkCreateDescriptorSetLayout(device, &create_info, nullptr, &sets_layout[BindlessSetType::GLOBAL]));
    }

    // Set 1 per instance uniforms
    //  Binding 0 Dynamic Uniform Buffer
    {
        VkDescriptorSetLayoutBinding set_layout_bindings[]{
            {
                .binding            = 0U,
                .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                .descriptorCount    = 1,
                .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
        };

        VkDescriptorSetLayoutCreateInfo create_info{
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext        = nullptr,
            .flags        = 0U,
            .bindingCount = sizeof(set_layout_bindings) / sizeof(VkDescriptorSetLayoutBinding),
            .pBindings    = set_layout_bindings,
        };

        VKCHECK(vkCreateDescriptorSetLayout(device, &create_info, nullptr, &sets_layout[BindlessSetType::INSTANCES_UNIFORMS]));
    }

    // Set 2 per draw uniforms
    //  Binding 0 Dynamic Uniform Buffer
    {
        VkDescriptorSetLayoutBinding set_layout_bindings[]{
            {
                .binding            = 0U,
                .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                .descriptorCount    = 1,
                .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
        };

        VkDescriptorSetLayoutCreateInfo create_info{
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext        = nullptr,
            .flags        = 0U,
            .bindingCount = sizeof(set_layout_bindings) / sizeof(VkDescriptorSetLayoutBinding),
            .pBindings    = set_layout_bindings,
        };

        VKCHECK(vkCreateDescriptorSetLayout(device, &create_info, nullptr, &sets_layout[BindlessSetType::DRAWS_UNIFORMS]));
    }

    VkPipelineLayoutCreateInfo create_info{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0U,
        .setLayoutCount         = sizeof(sets_layout) / sizeof(VkDescriptorSetLayout),
        .pSetLayouts            = sets_layout,
        .pushConstantRangeCount = 0U,
        .pPushConstantRanges    = nullptr,
    };

    VKCHECK(vkCreatePipelineLayout(device, &create_info, nullptr, &layout));
}

#ifndef USE_VK_DESCRIPTOR_BUFFER
void bindless_model::create_descriptor_pool(VkDevice device) {
    VkDescriptorPoolSize pool_sizes[]{
        {
            .type            = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = set_descriptors_count[BindlessSetType::MAX],
        },
        {
            .type            = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = set_descriptors_count[BindlessSetType::MAX],
        },
        {
            .type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = set_descriptors_count[BindlessSetType::MAX],
        },
        {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = set_descriptors_count[BindlessSetType::MAX],
        },
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info{
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0U,
        .maxSets       = descriptor_pool_allocable_sets_count,
        .poolSizeCount = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize),
        .pPoolSizes    = pool_sizes,
    };
    VKCHECK(vkCreateDescriptorPool(device, &descriptor_pool_create_info, nullptr, &descriptor_pool));
}

void bindless_model::allocate_sets(VkDevice device) {
    VkDescriptorSetAllocateInfo allocate_info{
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = nullptr,
        .descriptorPool     = descriptor_pool,
        .descriptorSetCount = 1U,
        .pSetLayouts        = &sets_layout[BindlessSetType::GLOBAL],
    };
    VKCHECK(vkAllocateDescriptorSets(device, &allocate_info, &sets[BindlessSetType::GLOBAL]));

    allocate_info.pSetLayouts        = &sets_layout[BindlessSetType::INSTANCES_UNIFORMS],
    VKCHECK(vkAllocateDescriptorSets(device, &allocate_info, &sets[BindlessSetType::INSTANCES_UNIFORMS]));

    allocate_info.pSetLayouts        = &sets_layout[BindlessSetType::DRAWS_UNIFORMS],
    VKCHECK(vkAllocateDescriptorSets(device, &allocate_info, &sets[BindlessSetType::DRAWS_UNIFORMS]));
}

void bindless_model::create_immutable_samplers(VkDevice device) {
    VkSamplerCreateInfo create_info{
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext                   = VK_NULL_HANDLE,
        .flags                   = 0,
        .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .mipLodBias              = 0.f,
        .anisotropyEnable        = VK_FALSE,
        .compareEnable           = VK_FALSE,
        .compareOp               = VK_COMPARE_OP_ALWAYS,
        .minLod                  = 0.f,
        .maxLod                  = 0.f,
        .borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    // Nearest sampler creation
    {
        VkFilter             filter       = VK_FILTER_NEAREST;
        VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        create_info.magFilter             = filter,
        create_info.minFilter             = filter,
        create_info.addressModeU          = address_mode,
        create_info.addressModeV          = address_mode,
        create_info.addressModeW          = address_mode,

        VKCHECK(vkCreateSampler(device, &create_info, VK_NULL_HANDLE, &samplers[SamplerType::NEAREST]));
    }

    // Linear sampler creation
    {
        VkFilter             filter       = VK_FILTER_LINEAR;
        VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        create_info.magFilter             = filter,
        create_info.minFilter             = filter,
        create_info.addressModeU          = address_mode,
        create_info.addressModeV          = address_mode,
        create_info.addressModeW          = address_mode,

        VKCHECK(vkCreateSampler(device, &create_info, VK_NULL_HANDLE, &samplers[SamplerType::LINEAR]));
    }
}

void bindless_model::destroy_immutable_samplers(VkDevice device) {
    for (auto* sampler : samplers) {
        vkDestroySampler(device, sampler, nullptr);
    }
}
#else
#endif
