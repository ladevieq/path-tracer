#pragma once

#include <string>

#include "vk-renderer.hpp"

class Renderpass {
    public:

    Renderpass(vkapi& api)
        :api(api)
    {};

    virtual ~Renderpass() {};

    // TODO: Do not use vulkan api type
    virtual void execute(vkrenderer& renderer, VkCommandBuffer command_buffer) = 0;

    protected:
    vkapi &api;
};
