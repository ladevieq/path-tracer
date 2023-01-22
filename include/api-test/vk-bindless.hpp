#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

enum BindlessSetType : uint32_t {
    GLOBAL,
    MAX,
};

enum SamplerType : uint32_t {
    NEAREST,
    LINEAR,
    MAX_SAMPLER,
};

struct bindless_model {
#ifndef USE_VK_DESCRIPTOR_BUFFERS
    VkPipelineLayout          layout;
    VkDescriptorSetLayout     sets_layout[BindlessSetType::MAX];
    VkDescriptorSet           sets[BindlessSetType::MAX];
    VkDescriptorPool          descriptor_pool;

    static constexpr size_t   descriptor_pool_allocable_sets_count = 64U;
    static constexpr uint32_t set_descriptors_count[BindlessSetType::MAX]{ 1024U };

    static bindless_model     create_bindless_model(VkDevice device);

    static void               destroy_bindless_model(VkDevice device, bindless_model& bindless);

    private:
    void create_layout(VkDevice device);
    void create_descriptor_pool(VkDevice device);
    void allocate_sets(VkDevice device);

    void destroy_descriptor_pool(VkDevice device);

    void create_immutable_samplers(VkDevice device);

    void destroy_immutable_samplers(VkDevice device);

    VkSampler samplers[SamplerType::MAX_SAMPLER];
#else
    VkPipelineLayout          layout;
    VkDescriptorSetLayout     sets_layout[BindlessSetType::MAX];

    static constexpr size_t   descriptor_pool_allocable_sets_count = 64U;
    static constexpr uint32_t set_descriptors_count[BindlessSetType::MAX]{ 1024U };

    static bindless_model     create_bindless_model(VkDevice device);

    static void               destroy_bindless_model(VkDevice device, bindless_model& bindless);

    private:
    void create_layout(VkDevice device);

    handle<device_buffer> storage_image_descriptors;
    handle<device_buffer> sampled_image_descriptors;
#endif
};
