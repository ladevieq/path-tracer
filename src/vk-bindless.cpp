#include "vk-bindless.hpp"

#include "vk-device.hpp"
#include "vk-utils.hpp"

device_buffer* bindless_model::get_uniform_buffer(enum UniformTypes type) const {
    return vkdevice::get().get_buffer(uniform_buffers[type]);
}

void bindless_model::init() {
    vkdevice& render_device = vkdevice::get();
    VkDevice device = render_device.get_device();

    // -------------------------
    // create immutable samplers
    // -------------------------
    {
        VkSamplerCreateInfo create_info{
            .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext                   = VK_NULL_HANDLE,
            .flags                   = 0,
            .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .mipLodBias              = 0.f,
            .anisotropyEnable        = VK_FALSE,
            .compareEnable           = VK_FALSE,
            .compareOp               = VK_COMPARE_OP_NEVER,
            .minLod                  = 0.f,
            .maxLod                  = 0.f,
            .borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };

        // Nearest sampler
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

        // Linear sampler
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
                .descriptorCount    = set_descriptors_count,
                .stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
            {
                .binding            = 2U,
                .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount    = set_descriptors_count,
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

        VKCHECK(vkCreateDescriptorSetLayout(device, &create_info, nullptr, &global_set_layout));
    }

    // ----------------------
    // create layout
    // ----------------------
    {
        VkPushConstantRange push_constant {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0U,
            .size = push_constant_slot_size * push_constant_slots,
        };

        VkPipelineLayoutCreateInfo create_info{
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0U,
            .setLayoutCount         = 1U,
            .pSetLayouts            = &global_set_layout,
            .pushConstantRangeCount = 1U,
            .pPushConstantRanges    = &push_constant,
        };

        VKCHECK(vkCreatePipelineLayout(device, &create_info, nullptr, &global_layout));
    }

    // ----------------------
    // create descriptor pool
    // ----------------------
    {
        VkDescriptorPoolSize pool_sizes[]{
            {
                .type            = VK_DESCRIPTOR_TYPE_SAMPLER,
                .descriptorCount = set_descriptors_count,
            },
            {
                .type            = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = set_descriptors_count,
            },
            {
                .type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = set_descriptors_count,
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

    // -----------------------
    // create descriptors sets
    // -----------------------
    {
        VkDescriptorSetAllocateInfo allocate_info{
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext              = nullptr,
            .descriptorPool     = descriptor_pool,
            .descriptorSetCount = 1U,
            .pSetLayouts        = &global_set_layout,
        };
        VKCHECK(vkAllocateDescriptorSets(device, &allocate_info, &global_set));
    }

    // ---------------------
    // global uniform buffer
    // ---------------------
    {
        uniform_buffers[UniformTypes::GLOBAL] = render_device.create_buffer({
            .size = globals_uniform_buffer_size,
            .usages = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            .name = "global uniform buffer",
        });
    }

    // ---------------------
    // passes uniform buffer
    // ---------------------
    {
        uniform_buffers[UniformTypes::PASS] = render_device.create_buffer({
            .size = passes_uniform_buffer_size,
            .usages = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            .name = "passes uniform buffer"
        });
    }

    // --------------------
    // draws uniform buffer
    // --------------------
    {
        uniform_buffers[UniformTypes::DRAW] = render_device.create_buffer({
            .size = draws_uniform_buffer_size,
            .usages = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            .name = "draws uniform buffer",
        });
    }
}

void bindless_model::deinit() {
    vkdevice& render_device = vkdevice::get();
    VkDevice device = render_device.get_device();

    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);

    vkDestroyDescriptorSetLayout(device, global_set_layout, nullptr);
    vkDestroyPipelineLayout(device, global_layout, nullptr);

    render_device.destroy_buffer(uniform_buffers[UniformTypes::GLOBAL]);
    render_device.destroy_buffer(uniform_buffers[UniformTypes::PASS]);
    render_device.destroy_buffer(uniform_buffers[UniformTypes::DRAW]);

    for (VkSampler sampler : samplers) {
        vkDestroySampler(device, sampler, nullptr);
    }
}
