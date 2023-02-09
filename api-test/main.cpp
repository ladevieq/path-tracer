#include <cassert>
#include <cstdio>
#include <span>
#include <array>
#include <vk_mem_alloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "imgui.h"

#include "handle.hpp"
#include "window.hpp"
#include "vk-device.hpp"
#include "utils.hpp"

struct device_texture;

#define ENABLE_RENDERDOC

#ifdef ENABLE_RENDERDOC
#include <renderdoc.h>

#define RD_START_CAPTURE if(rdoc_api != nullptr) rdoc_api->StartFrameCapture(nullptr, nullptr)
#define RD_END_CAPTURE if(rdoc_api != nullptr) rdoc_api->EndFrameCapture(nullptr, nullptr)

RENDERDOC_API_1_4_1 *rdoc_api = nullptr;
#endif

void ui() {
    ImGui::NewFrame();
    bool open = true;
    ImGui::ShowDemoWindow(&open);
    ImGui::EndFrame();
    ImGui::Render();
}

void update_buffers(ImDrawData* draw_data, const device_buffer& vertex_buffer, const device_buffer& index_buffer) {
    struct vertex {
        ImVec2 pos;
        ImVec2 uv;
        uint32_t color;
        uint32_t padding;
    };

    off_t vertex_offset = 0;
    off_t index_offset = 0;
    for (auto index {0U} ; index < draw_data->CmdListsCount; index++) {
        auto* cmd_list = draw_data->CmdLists[index];

        size_t vertex_bytes_size = sizeof(ImDrawVert) * cmd_list->VtxBuffer.size();
        auto* vtx_ptr = static_cast<vertex*>(vertex_buffer.mapped_ptr) + vertex_offset;
        for (auto vtx_index{0}; vtx_index < cmd_list->VtxBuffer.size(); vtx_index++, vtx_ptr++) {
            auto& vtx = cmd_list->VtxBuffer[vtx_index];
            vtx_ptr->pos = vtx.pos;
            vtx_ptr->uv = vtx.uv;
            vtx_ptr->color = vtx.col;
        }
        // memcpy(static_cast<uint8_t*>(vertex_buffer.mapped_ptr) + vertex_offset, cmd_list->VtxBuffer.Data, vertex_bytes_size);
        vertex_offset += static_cast<off_t>(vertex_bytes_size);

        size_t index_bytes_size = sizeof(ImDrawIdx) * cmd_list->IdxBuffer.size();
        memcpy(static_cast<uint8_t*>(index_buffer.mapped_ptr) + index_offset, cmd_list->IdxBuffer.Data, index_bytes_size);
        index_offset += static_cast<off_t>(index_bytes_size);
    }
}

int main() {

#ifdef ENABLE_RENDERDOC
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
      auto RENDERDOC_GetAPI =
          (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
      int ret =
          RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
      assert(ret == 1);
    }
#endif

    constexpr size_t window_width = 640U;
    constexpr size_t window_height = 360U;
    window wnd { window_width, window_height };

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize.x = window_width;
    io.DisplaySize.y = window_height;

    auto& device = vkdevice::get_render_device();
    device.init();

    auto surface_handle = device.create_surface({
        .window_handle = wnd.handle,
        .surface_format = {
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        },
        .present_mode = VK_PRESENT_MODE_FIFO_KHR,
        .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    });
    auto& surface = device.get_surface(surface_handle);

    constexpr size_t image_size = 1024U;
    constexpr size_t image_count = 4U;
    auto staging_buffer_handle = device.create_buffer({
        .size = image_size * image_size * sizeof(uint32_t) * image_count,
        .usages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    });
    const auto& staging_buffer = device.get_buffer(staging_buffer_handle);

    int width;
    int height;
    int channels;
    uint8_t* data = stbi_load("../models/sponza/466164707995436622.jpg", &width, &height, &channels, 4);
    size_t size = static_cast<size_t>(width) * height * 4U;
    memcpy(staging_buffer.mapped_ptr, data, size);

    auto gpu_texture_handle = device.create_texture({
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type   = VK_IMAGE_TYPE_2D,
    });

    off_t offset = width * height * 4;
    data = stbi_load("../models/sponza/715093869573992647.jpg", &width, &height, &channels, 4);
    size = static_cast<size_t>(width) * height * 4U;
    auto* addr = static_cast<void*>(static_cast<uint8_t*>(staging_buffer.mapped_ptr) + offset);
    memcpy(addr, data, size);

    auto second_texture = device.create_texture({
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type   = VK_IMAGE_TYPE_2D,
    });

    auto output_texture_handle = device.create_texture({
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .usages = VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type   = VK_IMAGE_TYPE_2D,
    });

    uint8_t *pixels = nullptr;
    int atlas_width;
    int atlas_height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &atlas_width, &atlas_height);
    const auto ui_texture_handle = device.create_texture({
        .width = static_cast<uint32_t>(atlas_width),
        .height = static_cast<uint32_t>(atlas_height),
        .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type = VK_IMAGE_TYPE_2D,
    });
    const auto& ui_texture = device.get_texture(ui_texture_handle);
    io.Fonts->SetTexID(*(void **)&ui_texture_handle.id);

    off_t ui_offset = 2L * offset;
    size = static_cast<size_t>(atlas_width) * atlas_height * 4U;
    addr = static_cast<void*>(static_cast<uint8_t*>(staging_buffer.mapped_ptr) + ui_offset);
    memcpy(addr, pixels, size);

    auto vertex_buffer = device.get_buffer(device.create_buffer({
        .size = 1024U * 1024U * sizeof(uint32_t),
        .usages = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    }));

    auto index_buffer_handle = device.create_buffer({
        .size = 1024U * 1024U * sizeof(uint32_t),
        .usages = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    });
    auto index_buffer = device.get_buffer(index_buffer_handle);

    auto code = read_file("shaders/test.comp.spv");
    auto compute = device.create_pipeline({
        .cs_code = code,
    });

    std::array<VkFormat, 1U> formats { surface.surface_format.format };
    auto vertex_code = read_file("shaders/api-test-ui.vert.spv");
    auto fragment_code = read_file("shaders/api-test-ui.frag.spv");
    auto graphics = device.create_pipeline({
        .vs_code = vertex_code,
        .fs_code = fragment_code,
        .color_attachments_format = formats
    });

    auto semaphore = device.create_semaphore({
        .type = VK_SEMAPHORE_TYPE_TIMELINE,
    });
    auto acquire_semaphore = device.create_semaphore({
        .type = VK_SEMAPHORE_TYPE_BINARY,
    });

    std::array<graphics_command_buffer, 2U> command_buffers;
    device.allocate_command_buffers(command_buffers.data(), 2, QueueType::GRAPHICS);

    RD_START_CAPTURE;

    command_buffers[0].start();

    command_buffers[0].barrier(gpu_texture_handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    command_buffers[0].barrier(second_texture, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    command_buffers[0].barrier(ui_texture_handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    command_buffers[0].copy(staging_buffer_handle, gpu_texture_handle);
    command_buffers[0].copy(staging_buffer_handle, second_texture, static_cast<VkDeviceSize>(offset));
    command_buffers[0].copy(staging_buffer_handle, ui_texture_handle, static_cast<VkDeviceSize>(ui_offset));

    command_buffers[0].barrier(gpu_texture_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
    command_buffers[0].barrier(second_texture, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

    struct compute_params {
        uint32_t input1;
        uint32_t input2;
        uint32_t output;
    };
    const auto& uniform_buffer = device.get_bindingmodel().get_uniform_buffer();
    compute_params params {
        device.get_texture(second_texture).storage_index,
        device.get_texture(gpu_texture_handle).storage_index,
        device.get_texture(output_texture_handle).storage_index,
    };
    memcpy(uniform_buffer.mapped_ptr, &params, sizeof(params));

    command_buffers[0].dispatch({
        .pipeline = compute,
        .group_size = { image_size, image_size, 1U },
        .local_group_size = { 8U, 8U, 1U },
        .uniforms_offset = 0U,
    });

    command_buffers[0].barrier(gpu_texture_handle, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    command_buffers[0].stop();

    std::array<struct command_buffer, 1U> compute_command_buffers = {command_buffers[0]};
    device.submit(compute_command_buffers, {}, semaphore);

    RD_END_CAPTURE;

    std::array<struct command_buffer, 1U> ui_command_buffers = {command_buffers[1]};
    // std::array<handle<device_texture>, 1U> color_attachment {gpu_texture_handle};
    surface.acquire_image_index(acquire_semaphore);
    std::array<handle<device_texture>, 1U> color_attachment {surface.get_backbuffer_image()};
    while(wnd.isOpen) {
        wnd.poll_events();

        ui();

        auto* draw_data = ImGui::GetDrawData();
        update_buffers(draw_data, vertex_buffer, index_buffer);

        device.wait(semaphore);

        RD_START_CAPTURE;
        command_buffers[1].start(),

        command_buffers[1].barrier(surface.get_backbuffer_image(), VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        command_buffers[1].begin_renderpass({
            .render_area = {
                .x = static_cast<int32_t>(draw_data->DisplayPos.x),
                .y = static_cast<int32_t>(draw_data->DisplayPos.y),
                .width = static_cast<uint32_t>(draw_data->DisplaySize.x),
                .height = static_cast<uint32_t>(draw_data->DisplaySize.y),
            },
            .color_attachments = color_attachment,
        });

        struct draw_params {
            uintptr_t vertex_buffer;
            float scale[2U];
            float translate[2U];
            uint32_t texture_index;
        };

        uint32_t draw_index = 0U;
        std::array<draw_params, 100U> draws;
        for (auto index {0U} ; index < draw_data->CmdListsCount; index++) {
            auto* cmd_list = draw_data->CmdLists[index];

            for (auto& draw_command : cmd_list->CmdBuffer) {
                draws[draw_index] = draw_params{
                    .vertex_buffer = vertex_buffer.device_address,
                    .scale {
                        2.f / draw_data->DisplaySize.x,
                        2.f / draw_data->DisplaySize.y,
                    },
                    .translate {
                        -1.f - draw_data->DisplayPos.x * (2.f / draw_data->DisplaySize.x),
                        -1.f - draw_data->DisplayPos.y * (2.f / draw_data->DisplaySize.y),
                    },
                    .texture_index = ui_texture.sampled_index,
                };

                command_buffers[1].render({
                    .pipeline = graphics,
                    .index_buffer = index_buffer_handle,
                    .vertex_count = draw_command.ElemCount,
                    .vertex_offset = draw_command.VtxOffset,
                    .index_offset = draw_command.IdxOffset,
                    .instance_count = 1U,
                    .uniforms_offset = 0U,
                });

                draw_index++;
            }
        }

        memcpy(uniform_buffer.mapped_ptr, draws.data(), sizeof(draw_params) * draw_index);

        command_buffers[1].end_renderpass();
        command_buffers[1].barrier(surface.get_backbuffer_image(), VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        command_buffers[1].stop();

        device.submit(ui_command_buffers, acquire_semaphore, semaphore);
        device.wait(semaphore);
        device.present(surface_handle, {});
        RD_END_CAPTURE;

        std::system("PAUSE");
    }

    return 0;
}
