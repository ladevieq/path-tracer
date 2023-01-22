#include "vk-bindless.hpp"

#include "vk-utils.hpp"
#include "vulkan-loader.hpp"

bindless_model bindless_model::create_bindless_model(VkDevice device) {
    bindless_model bindless;
#ifndef USE_VK_DESCRIPTOR_BUFFER
    bindless.create_immutable_samplers(device);
    bindless.create_layout(device);
    bindless.create_descriptor_pool(device);
    bindless.allocate_sets(device);
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

void bindless_model::destroy_bindless_model(VkDevice device, bindless_model& bindless) {
    vkDestroyDescriptorPool(device, bindless.descriptor_pool, nullptr);

    for (auto index{ 0U }; index < BindlessSetType::MAX; index++) {
        vkDestroyDescriptorSetLayout(device, bindless.sets_layout[index], nullptr);
    }

    vkDestroyPipelineLayout(device, bindless.layout, nullptr);

    bindless.destroy_immutable_samplers(device);
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
