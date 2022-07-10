#pragma once

#include "renderpass.hpp"
#include "vk-api.hpp"

class Texture;
class Buffer;

class ComputeRenderpass : public Renderpass {
    public:

    ComputeRenderpass(vkapi& api);
    ~ComputeRenderpass() override;

    void set_pipeline(const char* shader_name);

    void set_dispatch_size(size_t count_x, size_t count_y, size_t count_z);

    void set_ouput_texture(Texture* ouput_texture);

    void set_constant(off_t offset, uint64_t* constant);
    void set_constant(off_t offset, Texture* texture);
    void set_constant(off_t offset, Buffer* buffer);

    // TODO: Do not use vulkan api type
    void execute(vkrenderer& renderer, VkCommandBuffer command_buffer) final;

    Texture* output_texture;

    private:

    std::vector<Texture*> input_textures;

    uint8_t constants[64];

    size_t group_count_x;
    size_t group_count_y;
    size_t group_count_z;

    pipeline pipeline;
};
