#pragma once

#include <string>

#include "vk-api.hpp"

class Renderpass {
    public:

    Renderpass(vkapi& api)
        :api(api)
    {};

    virtual ~Renderpass() {
        api.destroy_pipeline(pipeline);
    }

    virtual void set_pipeline(std::string& shader_name) = 0;

    // TODO: Do not use vulkan api type
    virtual void execute(VkCommandBuffer command_buffer) = 0;

    protected:
    vkapi &api;

    pipeline pipeline;
};
