#pragma once

#include <vulkan/vulkan_core.h>

#include "handle.hpp"

enum UniformTypes : uint32_t {
    GLOBAL,
    PASS,
    DRAW,
    MAX,
};

enum SamplerType : uint32_t {
    NEAREST,
    LINEAR,
    MAX_SAMPLER,
};

struct device_buffer;

struct bindless_model {
    [[nodiscard]] device_buffer& get_uniform_buffer(enum UniformTypes type) const;

    void                    init();

    void                    deinit();

    VkPipelineLayout        global_layout;
    VkDescriptorSet         global_set;

public:
    static constexpr size_t push_constant_slot_size = sizeof(uintptr_t);
    static constexpr size_t push_constant_slots = 3U;

private:
    VkDescriptorSetLayout     global_set_layout;
    VkDescriptorPool          descriptor_pool;
    handle<device_buffer>     uniform_buffers[UniformTypes::MAX];

    static constexpr size_t   descriptor_pool_allocable_sets_count = 64U;
    static constexpr uint32_t set_descriptors_count = 1024U;

    VkSampler samplers[SamplerType::MAX_SAMPLER];

    static constexpr size_t globals_uniform_buffer_size = 64 * KB;
    static constexpr size_t passes_uniform_buffer_size = 64 * KB;
    static constexpr size_t draws_uniform_buffer_size = 64 * KB;
};
