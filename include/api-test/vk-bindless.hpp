#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "handle.hpp"

enum BindlessSetType : uint32_t {
    GLOBAL,
    INSTANCES_UNIFORMS,
    DRAWS_UNIFORMS,
    MAX,
};

enum ConstantType : uint32_t {
    GLOBALS,
    INSTANCES,
    DRAWS,
    VERTEX_BUFFER
};

enum SamplerType : uint32_t {
    NEAREST,
    LINEAR,
    MAX_SAMPLER,
};

struct device_buffer;

struct bindless_model {
#ifndef USE_VK_DESCRIPTOR_BUFFER
    [[nodiscard]] device_buffer& get_draws_uniform_buffer() const;
    [[nodiscard]] device_buffer& get_globals_uniform_buffer() const;

    static bindless_model     create_bindless_model();

    static void               destroy_bindless_model(bindless_model& bindless);

    VkPipelineLayout          graphics_layout;
    VkPipelineLayout          compute_layout;
    VkDescriptorSet           sets[BindlessSetType::MAX];

private:
    static constexpr size_t max_push_constants_size = 128ULL;

public:
    static constexpr size_t max_push_constants_slots_size = 8U;
    static constexpr size_t max_push_constants_slots = max_push_constants_size / max_push_constants_slots_size;

private:
    VkDescriptorSetLayout     sets_layout[BindlessSetType::MAX];
    VkDescriptorPool          descriptor_pool;
    handle<device_buffer>     globals_uniform_buffer_handle;
    handle<device_buffer>     instances_uniform_buffer_handle;
    handle<device_buffer>     draws_uniform_buffer_handle;

    static constexpr size_t   descriptor_pool_allocable_sets_count = 64U;
    static constexpr uint32_t set_descriptors_count = 1024U;

    void create_layout(VkDevice device);
    void create_descriptor_pool(VkDevice device);
    void allocate_sets(VkDevice device);

    void destroy_descriptor_pool(VkDevice device);

    void create_immutable_samplers(VkDevice device);

    void destroy_immutable_samplers(VkDevice device);

    VkSampler samplers[SamplerType::MAX_SAMPLER];

    static constexpr size_t globals_uniform_buffer_size = 64ULL * 1024ULL;
    static constexpr size_t instances_uniform_buffer_size = 64ULL * 1024ULL;
    static constexpr size_t draws_uniform_buffer_size = 64ULL * 1024ULL;
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
