// -------------------
// thirdparties
// -------------------
#define IMGUI_DISABLE_WIN32_FUNCTIONS
#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"

#include "simdjson.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define VK_NO_PROTOTYPES
#define VMA_VULKAN_VERSION 1004000
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
