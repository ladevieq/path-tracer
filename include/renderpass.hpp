#pragma once

using VkCommandBuffer = struct VkCommandBuffer_T*;

class vkrenderer;
class vkapi;

class Renderpass {
    public:

    Renderpass(vkapi& api)
        :api(api)
    {};

    virtual ~Renderpass() = default;

    // TODO: Do not use vulkan api type
    virtual void execute(vkrenderer& renderer, VkCommandBuffer command_buffer) = 0;

    protected:
    vkapi &api;
};
