// #include <cassert>
// #include <cstdio>
// #include <array>

#define STB_IMAGE_IMPLEMENTATION
// #include <stb_image.h>

#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"
#include "imgui_demo.cpp"

#define API_TEST

#include "../src/vulkan-loader.cpp"
#include "../src/window.cpp"
#include "../src/vec3.cpp"
#include "../src/camera.cpp"
#include "../src/scene.cpp"
#include "../src/utils.cpp"
#include "../src/bvh.cpp"
#include "../src/gltf.cpp"
#include "../src/mesh.cpp"

#include "vk-utils.cpp"
#include "vk-device.cpp"
#include "vk-bindless.cpp"
#include "vk-command-buffer.cpp"

// #include "handle.hpp"
// #include "window.hpp"
// #include "utils.hpp"

#ifdef VK
#include <vk_mem_alloc.h>
#include "vk-device.hpp"
#include "vk-command-buffer.hpp"
#else
#include "dx12-device.hpp"
#include "dx12-command-buffer.hpp"
#endif

struct device_texture;

#define VK
// #define DX12

#ifdef DEBUG
#define ENABLE_RENDERDOC
#endif // _DEBUG

#include "camera.hpp"
#include "scene.hpp"

#ifdef ENABLE_RENDERDOC
#include <renderdoc_app.h>

#define RD_START_CAPTURE if(rdoc_api != nullptr) rdoc_api->StartFrameCapture(nullptr, nullptr)
#define RD_END_CAPTURE if(rdoc_api != nullptr) rdoc_api->EndFrameCapture(nullptr, nullptr)

RENDERDOC_API_1_6_0 *rdoc_api = nullptr;
#else
#define RD_START_CAPTURE (0)
#define RD_END_CAPTURE (0)
#endif // ENABLE_RENDERDOC

static constexpr size_t Kb = 1024U;
static constexpr size_t Mb = 1024U * Kb;

void ui(uint64_t frame_time) {
    static float max_fps = 0.f;
    static uint64_t max_frame_time = 1000U;
    float fps = static_cast<float>(1000000U / frame_time);
    if (fps > max_fps) {
        max_fps = fps;
    }
    if (frame_time > max_frame_time) {
        max_frame_time = frame_time;
    }
    ImGui::NewFrame();
    bool open = true;
    ImGui::Text("FPS : %f\n", fps);
    ImGui::Text("Max FPS : %f\n", max_fps);
    ImGui::Text("frame_time : %f ms\n", frame_time / 1000.f);
    ImGui::Text("max frame_time : %f ms\n", max_frame_time / 1000.f);

    ImGui::Text("LUT");
    ImGui::Image(45, ImVec2(128, 128));
    ImGui::EndFrame();
    ImGui::Render();
}

void update_buffers(ImDrawData* draw_data, const device_buffer& vertex_buffer, const device_buffer& index_buffer) {
    off_t vertex_offset = 0;
    off_t index_offset = 0;
    for (auto index {0} ; index < draw_data->CmdListsCount; index++) {
        auto* cmd_list = draw_data->CmdLists[index];

        size_t vertex_bytes_size = sizeof(ImDrawVert) * cmd_list->VtxBuffer.size();
        auto* vtx_ptr = static_cast<uint8_t*>(vertex_buffer.mapped_ptr) + vertex_offset;
        memcpy(vtx_ptr, cmd_list->VtxBuffer.Data, vertex_bytes_size);
        vertex_offset += static_cast<off_t>(vertex_bytes_size);

        size_t index_bytes_size = sizeof(ImDrawIdx) * cmd_list->IdxBuffer.size();
        memcpy(static_cast<uint8_t*>(index_buffer.mapped_ptr) + index_offset, cmd_list->IdxBuffer.Data, index_bytes_size);
        index_offset += static_cast<off_t>(index_bytes_size);
    }
}

int main() {
    LARGE_INTEGER freq;
    // From Microsoft doc
    // https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency
    // The frequency of the performance counter is fixed at system boot and is
    // consistent across all processors so you only need to query the frequency
    // from QueryPerformanceFrequency as the application initializes,
    // and then cache the result.
    QueryPerformanceFrequency(&freq);

#ifdef ENABLE_RENDERDOC
    {
        HMODULE mod = GetModuleHandleA("renderdoc.dll");
        // if (!mod) {
        //     mod = LoadLibraryA("renderdoc.dll");
        // }

        if (mod) {
            auto RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
            assert(RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void **)&rdoc_api) == 1);
        }

        // if (!rdoc_api->IsTargetControlConnected()) {
        //     rdoc_api->LaunchReplayUI(true, nullptr);
        // }
    }
#endif

    constexpr size_t window_width = 1280U;
    constexpr size_t window_height = 720U;
    window wnd { window_width, window_height };

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize.x = window_width;
    io.DisplaySize.y = window_height;

    const float aspect_ratio = 16.f / 9.f;

    point3 position { 13.f, 2.f, -3.f };
    point3 target {};
    const auto v_fov = 90.f;
    const auto aperture = 0.1f;
    const auto focus_distance = 10.f;

#ifdef VK
    auto& device = vkdevice::get_render_device();
#else
    auto& device = dx12::device::get_render_device();
#endif
    device.init();

#ifdef VK
    auto main_scene = scene(camera(position, target, v_fov, aspect_ratio, aperture, focus_distance), window_width, window_height);
#endif

    auto surface_handle = device.create_surface({
        .window_handle = wnd.handle,
#ifdef VK
        .surface_format = {
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        },
        .present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR,
        .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
#else
        .surface_format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .present_mode = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .usages = DXGI_USAGE_RENDER_TARGET_OUTPUT,
#endif
        .image_count = 2U,
    });
    auto& surface = device.get_surface(surface_handle);

    constexpr size_t image_size = 1024U;
    constexpr size_t image_count = 4U;
    auto staging_buffer_handle = device.create_buffer({
        .size = image_size * image_size * sizeof(uint32_t) * image_count,
#ifdef VK
        .usages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
#else
        
        .map        = true,
        .states     = D3D12_RESOURCE_STATE_COPY_SOURCE,
        .usages     = D3D12_RESOURCE_FLAG_NONE,
        .heap_type  = D3D12_HEAP_TYPE_UPLOAD,
#endif
    });
    const auto& staging_buffer = device.get_buffer(staging_buffer_handle);

    // API test code
    int width;
    int height;
    int channels;
    uint8_t* data = stbi_load("../models/sponza/466164707995436622.jpg", &width, &height, &channels, 4);
    size_t size = static_cast<size_t>(width) * height * 4U;
    memcpy(staging_buffer.mapped_ptr, data, size);

    auto gpu_texture_handle = device.create_texture({
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
#ifdef VK
        .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type   = VK_IMAGE_TYPE_2D,
#else
        .states = D3D12_RESOURCE_STATE_COPY_DEST,
        .usages = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .type   = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
#endif
    });

    off_t buffer_offset = width * height * 4;
    data = stbi_load("../models/sponza/715093869573992647.jpg", &width, &height, &channels, 4);
    size = static_cast<size_t>(width) * height * 4U;
    auto* addr = static_cast<void*>(static_cast<uint8_t*>(staging_buffer.mapped_ptr) + buffer_offset);
    memcpy(addr, data, size);

    auto second_texture = device.create_texture({
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
#ifdef VK
        .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type   = VK_IMAGE_TYPE_2D,
#else
        .states = D3D12_RESOURCE_STATE_COPY_DEST,
        .usages = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .type   = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
#endif
    });

    auto output_texture_handle = device.create_texture({
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
#ifdef VK
        .usages = VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type   = VK_IMAGE_TYPE_2D,
#else
        .usages = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .type   = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
#endif
    });

    uint8_t *pixels = nullptr;
    int atlas_width;
    int atlas_height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &atlas_width, &atlas_height);
    const auto ui_texture_handle = device.create_texture({
        .width = static_cast<uint32_t>(atlas_width),
        .height = static_cast<uint32_t>(atlas_height),
#ifdef VK
        .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type = VK_IMAGE_TYPE_2D,
#else
        .states = D3D12_RESOURCE_STATE_COPY_DEST,
        .usages = D3D12_RESOURCE_FLAG_NONE,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .type = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
#endif
    });
    const auto& ui_texture = device.get_texture(ui_texture_handle);
    // io.Fonts->SetTexID(*(void **)&ui_texture_handle.id);
    io.Fonts->SetTexID(reinterpret_cast<void*>(ui_texture.get_sampled_index()));

    off_t ui_offset = 2L * buffer_offset;
    size = static_cast<size_t>(atlas_width) * atlas_height * 4U;
    addr = static_cast<void*>(static_cast<uint8_t*>(staging_buffer.mapped_ptr) + ui_offset);
    memcpy(addr, pixels, size);

    auto vertex_buffer_handle = device.create_buffer({
        .size = Mb * sizeof(uint32_t),
#ifdef VK
        .usages = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
#else
        #endif
    });
    auto vertex_buffer = device.get_buffer(vertex_buffer_handle);

    auto index_buffer_handle = device.create_buffer({
        .size = Mb * sizeof(uint32_t),
#ifdef VK
        .usages = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
#else
#endif
    });
    auto index_buffer = device.get_buffer(index_buffer_handle);

#ifdef VK
    auto code = read_file("shaders/test.comp.spv");
    auto compute = device.create_compute_pipeline({
        .cs_code = code,
    });

    auto rt_code = read_file("shaders/compute.comp.spv");
    auto raytracing = device.create_compute_pipeline({
        .cs_code = rt_code,
    });
    auto tonemapping_code = read_file("shaders/tonemapping.comp.spv");
    auto tonemapping = device.create_compute_pipeline({
        .cs_code = tonemapping_code,
    });
#endif
    const auto acc_handle = device.create_texture({
        .width = static_cast<uint32_t>(window_width),
        .height = static_cast<uint32_t>(window_height),
#ifdef VK
        .usages = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .type = VK_IMAGE_TYPE_2D,
#else
        .states = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .usages = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        .format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .type = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
#endif
    });

#ifdef VK
    auto acc_storage_index = device.get_texture(acc_handle).get_storage_index();
#endif

    const auto result_handle = device.create_texture({
        .width = static_cast<uint32_t>(window_width),
        .height = static_cast<uint32_t>(window_height),
#ifdef VK
        .usages = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .type = VK_IMAGE_TYPE_2D,
#else
        .states = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .usages = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        .format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .type = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
#endif
    });
#ifdef VK
    auto result_storage_index = device.get_texture(result_handle).get_storage_index();
#endif

#ifdef VK
    std::array<VkFormat, 1U> formats { surface.surface_format.format };
    auto vertex_code = read_file("shaders/api-test-ui.vert.spv");
    auto fragment_code = read_file("shaders/api-test-ui.frag.spv");
    auto graphics = device.create_graphics_pipeline({
        .vs_code = vertex_code,
        .fs_code = fragment_code,
        .color_attachments_format = formats
    });
#endif

#ifdef VK
    std::array<graphics_command_buffer, 4U> graphics_command_buffers;
    device.allocate_command_buffers(graphics_command_buffers.data(), graphics_command_buffers.size(), QueueType::GRAPHICS, "graphics_cmd_buf");
#else
    std::array<dx12::graphics_command_buffer, 4U> graphics_command_buffers;
    device.allocate_command_buffers(graphics_command_buffers.data(), graphics_command_buffers.size(), dx12::QueueType::GRAPHICS);
#endif


    handle<device_texture> transmittance_lut_handle = device.create_texture({
        .width  = static_cast<uint32_t>(1024),
        .height = static_cast<uint32_t>(1024),
#ifdef VK
        .usages = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .type   = VK_IMAGE_TYPE_2D,
#else
        .usages = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .type   = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
#endif
        .name   = "transmittance_lut",
    });
    device_texture transmittance_lut = device.get_texture(transmittance_lut_handle);

    std::vector<uint8_t> transmittance_lut_code = read_file("shaders/transmittance_lut.comp.spv");
    handle<device_pipeline> transmittance_lut_compute = device.create_compute_pipeline({
        .cs_code = transmittance_lut_code,
        .name = "transmittance_lut_comp",
    });


    // API test code
    RD_START_CAPTURE;

    graphics_command_buffers[0].start();

#ifdef VK
    graphics_command_buffers[0].barrier(gpu_texture_handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    graphics_command_buffers[0].barrier(second_texture, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    graphics_command_buffers[0].barrier(ui_texture_handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
#endif

    graphics_command_buffers[0].copy(staging_buffer_handle, gpu_texture_handle);
    graphics_command_buffers[0].copy(staging_buffer_handle, second_texture, buffer_offset);
    graphics_command_buffers[0].copy(staging_buffer_handle, ui_texture_handle, ui_offset);
#ifdef VK
    graphics_command_buffers[0].barrier(gpu_texture_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
    graphics_command_buffers[0].barrier(second_texture, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

    {
        graphics_command_buffers[0].dispatch({
            .pipeline = compute,
            .group_size = { .vec = { image_size, image_size, 1U }},
            .local_group_size = { .vec = { 8U, 8U, 1U }},
            .params = {
                device.get_texture(second_texture).get_storage_index(),         // Input 1
                device.get_texture(gpu_texture_handle).get_storage_index(),     // Input 2
                device.get_texture(output_texture_handle).get_storage_index(),  // Output
            },
        });

        graphics_command_buffers[0].dispatch({
            .pipeline = transmittance_lut_compute,
            .group_size = { .vec = { 1024U, 1024U, 1U }},
            .local_group_size = { .vec { 8U, 8U, 1U }},
            .params = {
                transmittance_lut.get_storage_index(),
            }
        });
    }

    graphics_command_buffers[0].barrier(gpu_texture_handle, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#endif

    graphics_command_buffers[0].stop();

    device.submit(graphics_command_buffers.data(), 1);
    device.wait();

    RD_END_CAPTURE;

    // watcher::watch_file(std::filesystem::path("../shaders/compute.comp"), [&]() {
    //     printf("change");
    // });

#ifdef VK
    std::array<handle<device_texture>, 1U> color_attachment {};
    constexpr uint32_t virtual_frames_count = 2U;
    uint64_t frame_time = 1U;
    uint32_t frame_count = 0U;
    while(wnd.isOpen) {
        LARGE_INTEGER start_qpc;
        QueryPerformanceCounter(&start_qpc);
        wnd.poll_events();

        watcher::pull_changes();

        uint32_t virtual_frame_index = frame_count % virtual_frames_count;
        auto command_buffer = graphics_command_buffers[2U + virtual_frame_index];
        for (auto& event : wnd.events) {
            if (event.type == EVENT_TYPES::MOUSE_MOVE) {
                io.MousePos.x = static_cast<float>(event.data.position.x);
                io.MousePos.y = static_cast<float>(event.data.position.y);
            }

            io.MouseDown[0] = event.type == EVENT_TYPES::BUTTON_PRESS;
        }

        ui(frame_time);

        auto* draw_data = ImGui::GetDrawData();
        update_buffers(draw_data, vertex_buffer, index_buffer);

        const auto& scene_buffer = device.get_buffer(main_scene.scene_buffer_handle);
        main_scene.meta.sample_index = frame_count;
        uint8_t* ptr = static_cast<uint8_t*>(scene_buffer.mapped_ptr);
        memcpy(static_cast<void*>(ptr), &main_scene.meta, sizeof(main_scene.meta));
        memcpy(static_cast<void*>(ptr + sizeof(main_scene.meta)), &main_scene.meta, sizeof(main_scene.meta));
        memcpy(static_cast<void*>(ptr + sizeof(main_scene.meta) * 2U), &main_scene.meta, sizeof(main_scene.meta));

        // TODO: Refactor backbuffer acquisition
        auto backbuffer = surface.swapchain_images[device.acquire_image_index(surface_handle)];
        color_attachment[0] = backbuffer;

        // RD_START_CAPTURE;
        command_buffer.start();

        command_buffer.barrier(backbuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);

        {
            uint64_t textures = (virtual_frame_index == 0U) ? ((uint64_t(acc_storage_index) << 32U) | result_storage_index) : ((uint64_t(result_storage_index) << 32U) | acc_storage_index);

            if (virtual_frame_index == 0) {
                command_buffer.barrier(result_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);
                command_buffer.barrier(acc_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL);
            } else {
                command_buffer.barrier(result_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL);
                command_buffer.barrier(acc_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);
            }

            command_buffer.dispatch({
                .pipeline = raytracing,
                .group_size = { .vec { window_width, window_height, 1U, } },
                .local_group_size = { .vec { 8U, 8U, 1U, } },
                .params = {
                    main_scene.scene_buffer_address(),
                    main_scene.bvh_buffer_address(),
                    main_scene.indices_buffer_address(),
                    main_scene.positions_buffer_address(),
                    main_scene.normals_buffer_address(),
                    main_scene.uvs_buffer_address(),
                    main_scene.materials_buffer_address(),
                    textures,
                },
            });

            // TODO: Refactor backbuffer acquisition
            auto backbuffer_storage_index = device.get_texture(backbuffer).get_storage_index();
            uint64_t out = (virtual_frame_index == 0U) ? ((uint64_t(result_storage_index) << 32U) | backbuffer_storage_index) : ((uint64_t(acc_storage_index) << 32U) | backbuffer_storage_index);
            command_buffer.dispatch({
                .pipeline = tonemapping,
                .group_size = { .vec { window_width, window_height, 1U, } },
                .local_group_size = { .vec { 8U, 8U, 1U, } },
                .params = {
                    main_scene.scene_buffer_address(),
                    main_scene.bvh_buffer_address(),
                    main_scene.indices_buffer_address(),
                    main_scene.positions_buffer_address(),
                    main_scene.normals_buffer_address(),
                    main_scene.uvs_buffer_address(),
                    main_scene.materials_buffer_address(),
                    out,
                },
            });
        }

        command_buffer.barrier(backbuffer, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        command_buffer.begin_renderpass({
            .render_area = {
                .x = static_cast<int32_t>(draw_data->DisplayPos.x),
                .y = static_cast<int32_t>(draw_data->DisplayPos.y),
                .width = static_cast<uint32_t>(draw_data->DisplaySize.x),
                .height = static_cast<uint32_t>(draw_data->DisplaySize.y),
            },
            .color_attachments = color_attachment,
        });

        auto& draws_uniform_buffer = device.get_bindingmodel().get_draws_uniform_buffer();
        uint32_t vertex_offset = 0;
        uint32_t index_offset = 0;
        for (auto index {0} ; index < draw_data->CmdListsCount; index++) {
            auto* cmd_list = draw_data->CmdLists[index];

            for (auto& draw_command : cmd_list->CmdBuffer) {
                auto offset = draws_uniform_buffer.offset;
                ImTextureID texture_id = draw_command.TexRef.GetTexID();
                {
                    struct ui_params {
                        // uintptr_t vertex_buffer;
                        float scale[2U];
                        float translate[2U];
                        uint32_t texture_index;
                    };
                    auto* ui_param = draws_uniform_buffer.allocate<ui_params>();
                    *ui_param = ui_params{
                        // .vertex_buffer = vertex_buffer.device_address,
                        .scale {
                            2.f / draw_data->DisplaySize.x,
                            2.f / draw_data->DisplaySize.y,
                        },
                        .translate {
                            -1.f - draw_data->DisplayPos.x * (2.f / draw_data->DisplaySize.x),
                            -1.f - draw_data->DisplayPos.y * (2.f / draw_data->DisplaySize.y),
                        },
                        .texture_index = *reinterpret_cast<uint32_t*>(&texture_id),
                    };
                }

                command_buffer.draw_indexed({
                    .pipeline = graphics,
                    .index_buffer = index_buffer_handle,
                    .vertex_count = draw_command.ElemCount,
                    .vertex_offset = draw_command.VtxOffset + vertex_offset,
                    .index_offset = draw_command.IdxOffset + index_offset,
                    .instance_count = 1U,
                    .uniforms_address = draws_uniform_buffer.device_address + offset,
                    .vertex_address = vertex_buffer.device_address,
                });
            }
            vertex_offset += cmd_list->VtxBuffer.size();
            index_offset += cmd_list->IdxBuffer.size();
        }

        command_buffer.end_renderpass();
        command_buffer.barrier(backbuffer, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        command_buffer.stop();

        device.submit_before_present(surface_handle, &command_buffer, 1U);
        device.present(surface_handle);
        device.wait();

        LARGE_INTEGER end_qpc;
        QueryPerformanceCounter(&end_qpc);
        LONGLONG diff_qpc = end_qpc.QuadPart - start_qpc.QuadPart;
        frame_time = (diff_qpc * 1000000ULL) / freq.QuadPart;
        frame_count++;

        // RD_END_CAPTURE;
    }

    device.wait();
    device.destroy_surface(surface_handle);

    device.destroy_pipeline(compute);
    device.destroy_pipeline(graphics);
    device.destroy_pipeline(raytracing);
    device.destroy_pipeline(tonemapping);

    device.destroy_texture(gpu_texture_handle);
    device.destroy_texture(second_texture);
    device.destroy_texture(output_texture_handle);
    device.destroy_texture(ui_texture_handle);
    device.destroy_texture(acc_handle);
    device.destroy_texture(result_handle);

    device.destroy_buffer(staging_buffer_handle);
    device.destroy_buffer(vertex_buffer_handle);
    device.destroy_buffer(index_buffer_handle);
    device.deinit();

    ImGui::DestroyContext();
#endif // VK

    return 0;
}
