// ===============
// windows.h stuff
// ===============
#if defined(_WIN32)
    #if defined(_M_AMD64) || defined(_M_X64)
        #define _AMD64_
    #endif
    #include <basetsd.h>
    #include <windef.h>
    #include <winnt.h>
    #include <winbase.h>
    #include <winuser.h>
    #include <winnls.h>
    #include <fileapi.h>
    #include <handleapi.h>
    #include <errhandlingapi.h>
#endif // _WIN32



// -------------------
// thirdparties
// -------------------
#include "imgui.h"

#include <stb_image.h>

#include <simdjson.h>

#define ENABLE_RENDERDOC 1
#if ENABLE_RENDERDOC
#include <renderdoc_app.h>


#define RD_START_CAPTURE if(rdoc_api != nullptr) rdoc_api->StartFrameCapture(nullptr, nullptr)
#define RD_END_CAPTURE if(rdoc_api != nullptr) rdoc_api->EndFrameCapture(nullptr, nullptr)
#else
#define RD_START_CAPTURE
#define RD_END_CAPTURE
#endif // ENABLE_RENDERDOC

#define VK 1
#define DX12 0
#if DX12
#include "dx12-device.hpp"
#include "dx12-command-buffer.hpp"
#else
#include "vulkan-loader.cpp"
#include "vk-utils.cpp"
#include "vk-device.cpp"
#include "vk-bindless.cpp"
#include "vk-command-buffer.cpp"
#endif

#include "vec3.cpp"
#include "utils.cpp"
#include "window.cpp"
#include "camera.cpp"
#include "mesh.cpp"
#include "gltf.cpp"
#include "bvh.cpp"
#include "scene.cpp"



// -------------------
// GLOBALS
// -------------------
LARGE_INTEGER freq;
std::vector<handle<device_texture>> debug_texture_list;

#if ENABLE_RENDERDOC
RENDERDOC_API_1_6_0 *rdoc_api = nullptr;
#endif // ENABLE_RENDERDOC

void watch_func() {
    printf("change");
}

void ui(scene* scene, float frame_time) {
    static float max_fps = 0.f;
    static float max_frame_time = 0.f;
    float fps = 1000U / (frame_time > 0.001f ? frame_time : 0.001f);
    if (fps > max_fps) {
        max_fps = fps;
    }
    if (frame_time > max_frame_time) {
        max_frame_time = frame_time;
    }

    ImGui::NewFrame();
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGuiWindowFlags wnd_flags = ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkSize.x - 250, main_viewport->WorkPos.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 720), ImGuiCond_FirstUseEver);

    ImGui::Begin("Debug", nullptr, wnd_flags);
    if (ImGui::CollapsingHeader("Perfs")) {
        ImGui::SeparatorText("Metrics");
        ImGui::Text("FPS : %f\n", fps);
        ImGui::Text("Max FPS : %f\n", max_fps);
        ImGui::Text("frame_time : %f ms\n", frame_time);
        ImGui::Text("max frame_time : %f ms\n", max_frame_time);

        ImGui::SeparatorText("Markers");
        for (uint32_t index = 0U; index < marker_count; index++ ) {
            uint64_t diff = marker_counter_end[index] - marker_counter_start[index];
            uint64_t time = (diff * 1000000ULL) / freq.QuadPart;

            ImGui::Text("%s time : %f ms\n", marker_names[index], static_cast<float>(time) / 1000U);
        }
    }

    if (ImGui::CollapsingHeader("Images")) {
        for (const handle<device_texture>& handle : debug_texture_list) {
            const device_texture* tex = vkdevice::get().get_texture(handle);
            const char* name = tex->name;
            ImGui::Text(name ? name : DUMMY_NAME);
            ImGui::Image(tex->get_sampled_index(), ImVec2(128, 128));
        }
    }

    if (ImGui::CollapsingHeader("Scene")) {
        ImGui::SliderInt("Max bounce", reinterpret_cast<int*>(&scene->meta.max_bounce), 5, 100);
        ImGui::SliderInt("Min bounce", reinterpret_cast<int*>(&scene->meta.min_bounce), 1, 5);

        ImGui::Checkbox("DOF", reinterpret_cast<bool*>(&scene->meta.enable_dof));
        ImGui::Checkbox("Debug BVH", reinterpret_cast<bool*>(&scene->meta.debug_bvh));
        if (ImGui::SliderInt("Scale facotr", reinterpret_cast<int*>(&scene->meta.downscale_factor), 1, 20)) {
            scene->meta.sample_index = 1U;
        }
    }

    ImGui::End();
    ImGui::EndFrame();
    ImGui::Render();
}

int main() {
    // From Microsoft doc
    // https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency
    // The frequency of the performance counter is fixed at system boot and is
    // consistent across all processors so you only need to query the frequency
    // from QueryPerformanceFrequency as the application initializes,
    // and then cache the result.
    QueryPerformanceFrequency(&freq);

    BEGIN_MARKER(STARTUP);

#if ENABLE_RENDERDOC
    {
        HMODULE mod = GetModuleHandleA("renderdoc.dll");
        if (!mod) {
            // https://github.com/baldurk/renderdoc/issues/1556
            //  If you load one copy of the dll yourself but a different copy of the dll is registered as a vulkan layer then both copies will be loaded because they're loaded from different paths.
            // They'll be mutually incompatible (you can't use the API fetched from one to capture from another), and other things may break or cause crashes
            mod = LoadLibraryA("C:/Program Files/RenderDoc/renderdoc.dll");

        }

        if (mod) {
            pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, (void **)&rdoc_api);
            assert(ret == 1);
            (void)ret;

            rdoc_api->MaskOverlayBits(eRENDERDOC_Overlay_All, eRENDERDOC_Overlay_All);

            RENDERDOC_InputButton key = RENDERDOC_InputButton::eRENDERDOC_Key_F10;
            rdoc_api->SetCaptureKeys(&key, 1U);
        }
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
    const float v_fov = 90.f;
    const float aperture = 0.1f;
    const float focus_distance = 10.f;

#if VK
    auto& device = vkdevice::get();
#else
    auto& device = dx12::device::get();
#endif
    device.init();

    scene main_scene = scene(camera(position, target, v_fov, aspect_ratio, aperture, focus_distance), window_width, window_height);

    handle<device_surface> surface_handle = device.create_surface({
        .window_handle = wnd.handle,
#if VK
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
    const device_surface* surface = device.get_surface(surface_handle);


    // ==============
    // create buffers
    // ==============
    const handle<device_buffer> staging_buffer_handle = device.create_buffer({
        .size = 128 * MB,
#if VK
        .usages = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
#else
        .map        = true,
        .states     = D3D12_RESOURCE_STATE_COPY_SOURCE,
        .usages     = D3D12_RESOURCE_FLAG_NONE,
        .heap_type  = D3D12_HEAP_TYPE_UPLOAD,
#endif
    });
    const device_buffer* staging_buffer = device.get_buffer(staging_buffer_handle);

    const handle<device_buffer> vertex_buffer_handle = device.create_buffer({
        .size = MB * sizeof(uint32_t),
#if VK
        .usages = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
#else
        #endif
    });

    const handle<device_buffer> index_buffer_handle = device.create_buffer({
        .size = MB * sizeof(uint32_t),
#if VK
        .usages = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
#else
#endif
    });

    device_buffer* vertex_buffer = device.get_buffer(vertex_buffer_handle);
    device_buffer* index_buffer = device.get_buffer(index_buffer_handle);


    // ================
    // create pipelines
    // ================
#if VK
    size_t code_size = 0U;
    uint8_t* code = read_file("shaders/test.comp.spv", &code_size);
    handle<device_pipeline> compute = device.create_compute_pipeline({
        .cs_code = code,
        .cs_code_size = code_size,
    });

    size_t rt_code_size = 0U;
    uint8_t* rt_code = read_file("shaders/compute.comp.spv", &rt_code_size);
    handle<device_pipeline> raytracing = device.create_compute_pipeline({
        .cs_code = rt_code,
        .cs_code_size = rt_code_size,
    });
    size_t tonemapping_code_size = 0U;
    uint8_t* tonemapping_code = read_file("shaders/tonemapping.comp.spv", &tonemapping_code_size);
    handle<device_pipeline> tonemapping = device.create_compute_pipeline({
        .cs_code = tonemapping_code,
        .cs_code_size = tonemapping_code_size,
    });

    std::array<VkFormat, 1U> formats { surface->surface_format.format };
    size_t vertex_code_size = 0U;
    uint8_t* vertex_code = read_file("shaders/api-test-ui.vert.spv", &vertex_code_size);
    size_t fragment_code_size = 0U;
    uint8_t* fragment_code = read_file("shaders/api-test-ui.frag.spv", &fragment_code_size);
    handle<device_pipeline> graphics = device.create_graphics_pipeline({
        .vs_code = vertex_code,
        .vs_code_size = vertex_code_size,
        .fs_code = fragment_code,
        .fs_code_size = fragment_code_size,
        .color_attachments_format = formats.data(),
        .color_attachments_format_count = formats.size()
    });

    size_t transmittance_lut_code_size = 0U;
    u8* transmittance_lut_code = read_file("shaders/transmittance_lut.comp.spv", &transmittance_lut_code_size);
    handle<device_pipeline> transmittance_lut_compute = device.create_compute_pipeline({
        .cs_code = transmittance_lut_code,
        .cs_code_size = transmittance_lut_code_size,
        .name = "transmittance_lut_comp",
    });
#endif

    // ==============
    // Image loading
    // ==============
    int image_width;
    int image_height;
    int channels;
    uint8_t* data = stbi_load("../models/sponza/466164707995436622.jpg", &image_width, &image_height, &channels, 4);
    size_t size = static_cast<size_t>(image_width) * image_height * 4U;
    memcpy(staging_buffer->mapped_ptr, data, size);

    off_t buffer_offset = image_width * image_height * 4;
    data = stbi_load("../models/sponza/715093869573992647.jpg", &image_width, &image_height, &channels, 4);
    size = static_cast<size_t>(image_width) * image_height * 4U;
    auto* addr = static_cast<void*>(static_cast<uint8_t*>(staging_buffer->mapped_ptr) + buffer_offset);
    memcpy(addr, data, size);

    u8 *pixels = nullptr;
    int atlas_width;
    int atlas_height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &atlas_width, &atlas_height);

    off_t ui_offset = 2L * buffer_offset;
    size = static_cast<size_t>(atlas_width) * atlas_height * 4U;
    addr = static_cast<void*>(static_cast<uint8_t*>(staging_buffer->mapped_ptr) + ui_offset);
    memcpy(addr, pixels, size);


    // ==============
    // create texures
    // ==============
    const handle<device_texture> gpu_texture_handle = device.create_texture({
        .width  = static_cast<uint32_t>(image_width),
        .height = static_cast<uint32_t>(image_height),
#if VK
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

    const handle<device_texture> second_texture = device.create_texture({
        .width  = static_cast<uint32_t>(image_width),
        .height = static_cast<uint32_t>(image_height),
#if VK
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

    const handle<device_texture> output_texture_handle = device.create_texture({
        .width  = static_cast<uint32_t>(image_width),
        .height = static_cast<uint32_t>(image_height),
#if VK
        .usages = VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type   = VK_IMAGE_TYPE_2D,
#else
        .usages = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .type   = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
#endif
    });

    const handle<device_texture> ui_texture_handle = device.create_texture({
        .width = static_cast<uint32_t>(atlas_width),
        .height = static_cast<uint32_t>(atlas_height),
#if VK
        .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type = VK_IMAGE_TYPE_2D,
#else
        .states = D3D12_RESOURCE_STATE_COPY_DEST,
        .usages = D3D12_RESOURCE_FLAG_NONE,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .type = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
#endif
        .name = "glyph atlas"
    });

    const handle<device_texture> acc_handle = device.create_texture({
        .width = static_cast<uint32_t>(window_width),
        .height = static_cast<uint32_t>(window_height),
#if VK
        .usages = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .type = VK_IMAGE_TYPE_2D,
#else
        .states = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .usages = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        .format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .type = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
#endif
        .name = "accumulation texture"
    });

    const handle<device_texture> result_handle = device.create_texture({
        .width = static_cast<uint32_t>(window_width),
        .height = static_cast<uint32_t>(window_height),
#if VK
        .usages = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .type = VK_IMAGE_TYPE_2D,
#else
        .states = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        .usages = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        .format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .type = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
#endif
        .name = "output texture"
    });

    const handle<device_texture> transmittance_lut_handle = device.create_texture({
        .width  = static_cast<uint32_t>(1024),
        .height = static_cast<uint32_t>(1024),
#if VK
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

#if VK
    u32 acc_storage_index = device.get_texture(acc_handle)->get_storage_index();
    u32 result_storage_index = device.get_texture(result_handle)->get_storage_index();
#endif

    const device_texture* ui_texture = device.get_texture(ui_texture_handle);
    const device_texture* transmittance_lut = device.get_texture(transmittance_lut_handle);
    debug_texture_list.push_back(transmittance_lut_handle);

    io.Fonts->SetTexID(reinterpret_cast<void*>(static_cast<uintptr_t>(ui_texture->get_sampled_index())));


#if VK
    std::array<graphics_command_buffer, 4U> graphics_command_buffers;
    device.allocate_command_buffers(graphics_command_buffers.data(), graphics_command_buffers.size(), QueueType::GRAPHICS, "graphics_cmd_buf");
#else
    std::array<dx12::graphics_command_buffer, 4U> graphics_command_buffers;
    device.allocate_command_buffers(graphics_command_buffers.data(), graphics_command_buffers.size(), dx12::QueueType::GRAPHICS);
#endif


    // =============
    // API test code
    // =============
    RD_START_CAPTURE;
    graphics_command_buffers[0].start();

#if VK
    graphics_command_buffers[0].barrier(gpu_texture_handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    graphics_command_buffers[0].barrier(second_texture, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    graphics_command_buffers[0].barrier(ui_texture_handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
#endif

    graphics_command_buffers[0].copy(staging_buffer_handle, gpu_texture_handle);
    graphics_command_buffers[0].copy(staging_buffer_handle, second_texture, buffer_offset);
    graphics_command_buffers[0].copy(staging_buffer_handle, ui_texture_handle, ui_offset);
#if VK
    graphics_command_buffers[0].barrier(gpu_texture_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
    graphics_command_buffers[0].barrier(second_texture, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);


    {
        // device_buffer* draws_uniform_buffer = device.get_bindingmodel().get_uniform_buffer(UniformTypes::DRAW);
        {
            struct params {
                u32 input1;
                u32 input2;
                u32 output;
                u32 pad;
            // } *params = draws_uniform_buffer->allocate<struct params>();
            } params;
             // params->input1 = device.get_texture(second_texture)->get_storage_index();
             // params->input2 = device.get_texture(gpu_texture_handle)->get_storage_index();
             // params->output = device.get_texture(output_texture_handle)->get_storage_index();
             params.input1 = device.get_texture(second_texture)->get_storage_index();
             params.input2 = device.get_texture(gpu_texture_handle)->get_storage_index();
             params.output = device.get_texture(output_texture_handle)->get_storage_index();

            graphics_command_buffers[0].dispatch({
                .pipeline = compute,
                .group_size = { .vec = { static_cast<u32>(image_width), static_cast<u32>(image_height), 1U }},
                .local_group_size = { .vec = { 8U, 8U, 1U }},
            },
            &params, sizeof(struct params));
        }

        {
            struct params {
                u32 lut_id;
                u32 width;
                u32 height;
                u32 pad;
            // } *params = draws_uniform_buffer->allocate<struct params>();
            } params;
             // params->lut_id = transmittance_lut->get_storage_index();
             // params->width = transmittance_lut->width;
             // params->height = transmittance_lut->height;
             params.lut_id = transmittance_lut->get_storage_index();
             params.width = transmittance_lut->width;
             params.height = transmittance_lut->height;

            graphics_command_buffers[0].dispatch({
                .pipeline = transmittance_lut_compute,
                .group_size = { .vec = { 1024U, 1024U, 1U }},
                .local_group_size = { .vec { 8U, 8U, 1U }},
            },
            &params, sizeof(struct params));
        }
    }

    graphics_command_buffers[0].barrier(gpu_texture_handle, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
#endif

    graphics_command_buffers[0].stop();

    device.submit(graphics_command_buffers.data(), 1);
    device.wait();

    RD_END_CAPTURE;

    END_MARKER(STARTUP);

    watch_file("../shaders/compute.comp", watch_func);

#if VK
    std::array<handle<device_texture>, 1U> color_attachment {};
    constexpr uint32_t virtual_frames_count = 2U;
    float delta = 0.f;          // milliseconds
    u32 speed = 1U;
    uint32_t frame_count = 0U;
    while(wnd.isOpen) {
        vec3 dir = { 0.f, 0.f, 0.f };
        float rot = 0.f;

        LARGE_INTEGER start_qpc;
        QueryPerformanceCounter(&start_qpc);
        wnd.poll_events();

        pull_changes();

        uint32_t virtual_frame_index = frame_count % virtual_frames_count;
        auto command_buffer = graphics_command_buffers[2U + virtual_frame_index];

        main_scene.meta.sample_index++;

        //
        // IOs
        //
        {
            io.AddMouseButtonEvent(BUTTON::LEFT, wnd.mouse[BUTTON::LEFT]);
            io.AddMouseButtonEvent(BUTTON::RIGHT, wnd.mouse[BUTTON::LEFT]);
            io.AddMouseButtonEvent(BUTTON::MIDDLE, wnd.mouse[BUTTON::LEFT]);
            for (event& event : wnd.events) {
                if (event.type == EVENT_TYPE::MOUSE_MOVE) {
                    io.AddMousePosEvent(static_cast<float>(event.data.position.x), static_cast<float>(event.data.position.y));
                } else if (event.type == EVENT_TYPE::MOUSE_WHEEL) {
                    io.AddMouseWheelEvent(static_cast<float>(event.data.delta.x), static_cast<float>(event.data.delta.y));
                }

                if (event.type == EVENT_TYPE::KEY_PRESS) {
                    if (event.data.key == KEY::F2) {
#if ENABLE_RENDERDOC
                        if (!rdoc_api->IsTargetControlConnected()) {
                            rdoc_api->LaunchReplayUI(true, nullptr);
                        }
#endif
                    }
                }

                if (event.type == EVENT_TYPE::KEY_DOWN) {
                    if (event.data.key == KEY::W) {
                        dir += main_scene.meta.cam.forward;
                    }
                    if (event.data.key == KEY::A) {
                        dir += -main_scene.meta.cam.right;
                    }
                    if (event.data.key == KEY::S) {
                        dir += -main_scene.meta.cam.forward;
                    }
                    if (event.data.key == KEY::D) {
                        dir += main_scene.meta.cam.right;
                    }
                    if (event.data.key == KEY::SPACE) {
                        dir += vec3 { 0.f, 1.f, 0.f };
                    }
                    if (event.data.key == KEY::LCTRL) {
                        dir += vec3 { 0.f, -1.f, 0.f };
                    }
                    if (event.data.key == KEY::E) {
                        rot += 0.001f;
                    }
                    if (event.data.key == KEY::Q) {
                        rot += -0.001f;
                    }
                    if (event.data.key == KEY::PAGE_UP) {
                        speed++;
                    }
                    if (event.data.key == KEY::PAGE_DOWN) {
                        if (speed > 1U) {
                            speed--;
                        }
                    }

                }
            }
        }

        if (!dir.is_zero()) {
            main_scene.meta.cam.move(dir.normalize() * delta * static_cast<float>(speed));
            main_scene.meta.sample_index = 1U;
        }
        if (rot != 0.f) {
            main_scene.meta.cam.rotate_y(rot * delta * static_cast<float>(speed));
            main_scene.meta.sample_index = 1U;
        }

        ui(&main_scene, delta);

        ImGuiMouseCursor cursor_style = ImGui::GetMouseCursor();
        window::set_cursor(static_cast<CURSOR_TYPE>(cursor_style));

        // ==================
        // copy ImGui buffers
        // ==================
        const ImDrawData* draw_data = ImGui::GetDrawData();
        {
            u32 vertex_offset = 0;
            u32 index_offset = 0;
            for (s32 index = 0U; index < draw_data->CmdListsCount; index++) {
                ImDrawList* cmd_list = draw_data->CmdLists[index];

                size_t vertex_bytes_size = sizeof(ImDrawVert) * cmd_list->VtxBuffer.size();
                u8* vtx_ptr = static_cast<uint8_t*>(vertex_buffer->mapped_ptr) + vertex_offset;
                memcpy(vtx_ptr, cmd_list->VtxBuffer.Data, vertex_bytes_size);
                vertex_offset += static_cast<off_t>(vertex_bytes_size);

                size_t index_bytes_size = sizeof(ImDrawIdx) * cmd_list->IdxBuffer.size();
                memcpy(static_cast<uint8_t*>(index_buffer->mapped_ptr) + index_offset, cmd_list->IdxBuffer.Data, index_bytes_size);
                index_offset += static_cast<off_t>(index_bytes_size);
            }
        }

        // ======================
        // global uniforms update
        // ======================
        {
            // metadata* const globals = vkdevice::get().get_bindingmodel().get_uniform_buffer(UniformTypes::GLOBAL)->allocate<metadata>();
            // memcpy(static_cast<void*>(globals), &main_scene.meta, sizeof(main_scene.meta));
        }

        // TODO: Refactor backbuffer acquisition
        auto backbuffer = surface->swapchain_images[device.acquire_image_index(surface_handle)];
        color_attachment[0] = backbuffer;

        // RD_START_CAPTURE;
        command_buffer.start();

        command_buffer.barrier(backbuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);

        {
            if (virtual_frame_index == 0) {
                command_buffer.barrier(result_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);
                command_buffer.barrier(acc_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL);
            } else {
                command_buffer.barrier(result_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL);
                command_buffer.barrier(acc_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);
            }

            {
                struct params {
                    uintptr_t bvh_buffer;
                    uintptr_t indices_buffer;
                    uintptr_t positions_buffer;
                    uintptr_t normals_buffer;
                    uintptr_t uvs_buffer;
                    uintptr_t materials_buffer;
                    u32 output_id;
                    u32 accumulation_id;
                };
                // struct params* uniforms = device.get_bindingmodel().get_uniform_buffer(UniformTypes::DRAW)->allocate<params>();
                // uniforms->bvh_buffer = main_scene.bvh_buffer_address();
                // uniforms->indices_buffer = main_scene.indices_buffer_address();
                // uniforms->positions_buffer = main_scene.positions_buffer_address();
                // uniforms->normals_buffer = main_scene.normals_buffer_address();
                // uniforms->uvs_buffer = main_scene.uvs_buffer_address();
                // uniforms->materials_buffer = main_scene.materials_buffer_address();
                // uniforms->output_id = (virtual_frame_index == 0U) ? result_storage_index : acc_storage_index;
                // uniforms->accumulation_id = (virtual_frame_index == 0U) ? acc_storage_index : result_storage_index;
                struct params uniforms = {};
                uniforms.bvh_buffer = main_scene.bvh_buffer_address();
                uniforms.indices_buffer = main_scene.indices_buffer_address();
                uniforms.positions_buffer = main_scene.positions_buffer_address();
                uniforms.normals_buffer = main_scene.normals_buffer_address();
                uniforms.uvs_buffer = main_scene.uvs_buffer_address();
                uniforms.materials_buffer = main_scene.materials_buffer_address();
                uniforms.output_id = (virtual_frame_index == 0U) ? result_storage_index : acc_storage_index;
                uniforms.accumulation_id = (virtual_frame_index == 0U) ? acc_storage_index : result_storage_index;

                command_buffer.dispatch({
                    .pipeline = raytracing,
                    .group_size = { .vec { window_width / main_scene.meta.downscale_factor, window_height / main_scene.meta.downscale_factor, 1U, } },
                    .local_group_size = { .vec { 8U, 8U, 1U, } },
                }, (const void*)&uniforms, sizeof(struct params));
            }

            {
                struct params {
                    u32 output_id;
                    u32 accumulation_id;
                // } *uniforms = device.get_bindingmodel().get_uniform_buffer(UniformTypes::DRAW)->allocate<params>();
                } uniforms;
                // TODO: Refactor backbuffer acquisition
                // uniforms->output_id = device.get_texture(backbuffer)->get_storage_index();
                // uniforms->accumulation_id = (virtual_frame_index == 0U) ? result_storage_index : acc_storage_index;
                uniforms.output_id = device.get_texture(backbuffer)->get_storage_index();
                uniforms.accumulation_id = (virtual_frame_index == 0U) ? result_storage_index : acc_storage_index;

                command_buffer.dispatch({
                    .pipeline = tonemapping,
                    .group_size = { .vec { window_width / main_scene.meta.downscale_factor, window_height / main_scene.meta.downscale_factor, 1U, } },
                    .local_group_size = { .vec { 8U, 8U, 1U, } },
                }, &uniforms, sizeof(struct params));
            }
        }

        command_buffer.barrier(backbuffer, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        command_buffer.begin_renderpass({
            .render_area = {
                .x = static_cast<int32_t>(draw_data->DisplayPos.x),
                .y = static_cast<int32_t>(draw_data->DisplayPos.y),
                .width = static_cast<uint32_t>(draw_data->DisplaySize.x),
                .height = static_cast<uint32_t>(draw_data->DisplaySize.y),
            },
            .color_attachments = color_attachment.data(),
            .color_attachments_count = color_attachment.size(),
        });

        // device_buffer* draws_uniform_buffer = device.get_bindingmodel().get_uniform_buffer(UniformTypes::DRAW);
        uint32_t vertex_offset = 0;
        uint32_t index_offset = 0;
        for (s32 index = 0U ; index < draw_data->CmdListsCount; index++) {
            auto* cmd_list = draw_data->CmdLists[index];

            for (auto& draw_command : cmd_list->CmdBuffer) {
                ImTextureID texture_id = draw_command.TexRef.GetTexID();
                struct ui_params {
                    u64 vertex_buffer;
                    float scale[2U];
                    float translate[2U];
                    uint32_t texture_index;
                    u32 padding;
                };
                // struct ui_params* ui_param = draws_uniform_buffer->allocate<ui_params>();
                // *ui_param = ui_params{
                struct ui_params ui_param = ui_params{
                    .vertex_buffer = vertex_buffer->device_address,
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

                command_buffer.draw_indexed({
                    .pipeline = graphics,
                    .index_buffer = index_buffer_handle,
                    .vertex_count = draw_command.ElemCount,
                    .vertex_offset = draw_command.VtxOffset + vertex_offset,
                    .index_offset = draw_command.IdxOffset + index_offset,
                    .instance_count = 1U,
                }, &ui_param, sizeof(ui_param));
            }
            vertex_offset += cmd_list->VtxBuffer.size();
            index_offset += cmd_list->IdxBuffer.size();
        }

        command_buffer.end_renderpass();
        command_buffer.barrier(backbuffer, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        command_buffer.stop();

        device.submit_before_present(surface_handle, &command_buffer, 1U);
        device.present(surface_handle);
        // device.wait();

        LARGE_INTEGER end_qpc;
        QueryPerformanceCounter(&end_qpc);
        LONGLONG diff_qpc = end_qpc.QuadPart - start_qpc.QuadPart;
        delta = static_cast<float>(diff_qpc * 1000LL) / freq.QuadPart;
        frame_count++;

        // RD_END_CAPTURE;
    }

    device.wait();

    main_scene.deinit();

    device.destroy_surface(surface_handle);

    device.destroy_pipeline(compute);
    device.destroy_pipeline(graphics);
    device.destroy_pipeline(raytracing);
    device.destroy_pipeline(tonemapping);

    device.destroy_pipeline(transmittance_lut_compute);
    device.destroy_texture(transmittance_lut_handle);

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
