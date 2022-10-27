#pragma once

#include <vulkan/vulkan_core.h>

class command_buffer {
    void start();
    void stop();

    VkCommandBuffer handle;
};

class graphics_command_buffer : command_buffer {

};
