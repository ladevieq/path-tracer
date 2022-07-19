#include <chrono>
#include <filesystem>

#include "imgui.h"

#ifdef ENABLE_RENDERDOC
#include <renderdoc.h>
#endif

#include "compute-renderpass.hpp"
#include "primitive-renderpass.hpp"
#include "scene.hpp"
#include "vk-renderer.hpp"
#include "window.hpp"

#include "Tracy.hpp"

#if defined(LINUX)
#include <dlfcn.h>
#include <unistd.h>
#elif defined(WINDOWS)
#include <Windows.h>
#endif

struct VkVertex {
    float x, y;
    float u, v;
    uint32_t color;
    uint32_t padding[3];
};

int main() {
#ifdef ENABLE_RENDERDOC
    RENDERDOC_API_1_1_2 *rdoc_api = nullptr;

#ifdef WINDOWS
    // At init, on windows
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
      auto RENDERDOC_GetAPI =
          (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
      int ret =
          RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
      assert(ret == 1);
    }
#elif defined(LINUX)
    // At init, on linux/android.
    // For android replace librenderdoc.so with libVkLayer_GLES_RenderDoc.so
    if (void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD)) {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
        assert(ret == 1);
    }
#endif
#endif // ENABLE_RENDERDOC

    const float aspect_ratio = 16.0 / 9.0;
    const auto width = 400;
    auto height = (uint32_t)(width / aspect_ratio);

    window wnd { width, height };

    float delta_time = 0.f;
    uint64_t frame_count = 0;
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();

    point3 position { 13.f, 2.f, -3.f };
    point3 target {};
    const auto v_fov = 90.f;
    const auto aperture = 0.1f;
    const auto focus_distance = 10.f;
    auto cam = camera(position, target, v_fov, aspect_ratio, aperture, focus_distance);

    vkrenderer renderer { wnd };

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    auto main_scene = scene(cam, width, height, renderer);

    auto *raytracing_pass = renderer.create_compute_renderpass();
    raytracing_pass->set_pipeline(raytracing_shader_name);

    auto *accumulation_texture = renderer.create_2d_texture(width, height, VK_FORMAT_R32G32B32A32_SFLOAT);
    auto *output_texture = renderer.create_2d_texture(width, height, VK_FORMAT_R32G32B32A32_SFLOAT);
    raytracing_pass->set_constant(8, main_scene.bvh_buffer);
    raytracing_pass->set_constant(16, main_scene.indices_buffer);
    raytracing_pass->set_constant(24, main_scene.positions_buffer);
    raytracing_pass->set_constant(32, main_scene.normals_buffer);
    raytracing_pass->set_constant(40, main_scene.uvs_buffer);
    raytracing_pass->set_constant(48, main_scene.materials_buffer);
    raytracing_pass->set_dispatch_size(width / 8 + 1, height / 8 + 1, 1);

    uint8_t *pixels = nullptr;
    int atlas_width, atlas_height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &atlas_width, &atlas_height);
    auto *ui_texture = renderer.create_2d_texture(atlas_width, atlas_height, VK_FORMAT_R8G8B8A8_UNORM);
    io.Fonts->SetTexID((void *)&ui_texture);

    renderer.update_image(ui_texture, pixels);
    auto ui_texture_sampler = renderer.create_sampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    Buffer* ui_vertex_buffer = nullptr;
    Buffer* ui_index_buffer = nullptr;

    std::string ui_shader_name = "ui";
    std::vector<Primitive*> ui_primitives;
    auto ui_pass = renderer.create_primitive_renderpass();
    ui_pass->set_viewport(0.f, 0.f, (float)width, (float)height);
    ui_pass->add_color_attachment(renderer.back_buffer_format());
    ui_pass->finalize_render_pass();

    auto can_render = true;

    io.DisplaySize.x = (float)width;
    io.DisplaySize.y = (float)height;

    while (wnd.isOpen) {
        end = std::chrono::high_resolution_clock::now();

        delta_time = std::chrono::duration<float, std::milli>(end - start).count();

        start = std::chrono::high_resolution_clock::now();

        io.DeltaTime = delta_time;

        wnd.poll_events();

        for (auto &event : wnd.events) {
            switch (event.type) {
            case EVENT_TYPES::RESIZE: {
                main_scene.meta.width = event.width;
                main_scene.meta.height = event.height;

                io.DisplaySize.x = (float)event.width;
                io.DisplaySize.y = (float)event.height;

                if (event.width == 0 && event.height == 0) {
                    can_render = false;
                } else {
                    can_render = true;
                    renderer.recreate_swapchain();
                    main_scene.meta.cam.set_aspect_ratio((float)event.width / (float)event.height);

                    renderer.destroy_2d_texture(accumulation_texture);
                    renderer.destroy_2d_texture(output_texture);
                    accumulation_texture = renderer.create_2d_texture(event.width, event.height, VK_FORMAT_R32G32B32A32_SFLOAT);
                    output_texture = renderer.create_2d_texture(event.width, event.height, VK_FORMAT_R32G32B32A32_SFLOAT);
                    raytracing_pass->set_dispatch_size(event.width / 8 + 1, event.height / 8 + 1, 1);
                }

                main_scene.meta.sample_index = 1;

                break;
            }
            case EVENT_TYPES::KEY_PRESS: {
                auto move_speed = 1.f * delta_time;
                if (wnd.keyboard[KEYS::LSHIFT] || wnd.keyboard[KEYS::RSHIFT]) { move_speed *= 10.f; }

                vec3 move_vec{};

                switch (event.key) {
                case KEYS::W: {
                    move_vec = main_scene.meta.cam.forward * move_speed;
                    break;
                }

                case KEYS::S: {
                    move_vec = -main_scene.meta.cam.forward * move_speed;
                    break;
                }
                case KEYS::D: {
                    move_vec = main_scene.meta.cam.right * move_speed;
                    break;
                }
                case KEYS::A: {
                    move_vec = -main_scene.meta.cam.right * move_speed;
                    break;
                }
                case KEYS::E: {
                    main_scene.meta.cam.rotate_y(0.001f * delta_time);
                    main_scene.meta.sample_index = 1;
                    break;
                }
                case KEYS::Q: {
                    main_scene.meta.cam.rotate_y(-0.001f * delta_time);
                    main_scene.meta.sample_index = 1;
                    break;
                }
                case KEYS::SPACE: {
                    move_vec = main_scene.meta.cam.up * move_speed;
                    break;
                }
                case KEYS::LCTRL: {
                    move_vec = -main_scene.meta.cam.up * move_speed;
                    break;
                }
                default:
                    break;
                }

                main_scene.meta.cam.move(move_vec);

                if (!move_vec.near_zero()) { main_scene.meta.sample_index = 1; }

                break;
            }
            case EVENT_TYPES::BUTTON_PRESS: {
                if (event.button == BUTTONS::LEFT) { io.MouseDown[0] = true; }
                if (event.button == BUTTONS::RIGHT) { io.MouseDown[1] = true; }
                break;
            }
            case EVENT_TYPES::BUTTON_RELEASE: {
                if (event.button == BUTTONS::LEFT) { io.MouseDown[0] = false; }
                if (event.button == BUTTONS::RIGHT) { io.MouseDown[1] = false; }
                break;
            }
            case EVENT_TYPES::MOUSE_MOVE: {
                io.MousePos = ImVec2{(float)event.x, (float)event.y};
                break;
            }
            default:
                break;
            }
        }

        wnd.events.clear();

        // GUI
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({0.f, 0.f});
        ImGui::SetNextWindowSize({0.f, 0.f});
        ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        ImGui::Text("frame per second %u\n", static_cast<uint32_t>(1000.f / delta_time));
        ImGui::Text("frame time %f ms\n", delta_time);
        ImGui::Text("frame count %u\n", main_scene.meta.sample_index);

        if (ImGui::Checkbox("depth of field", (bool *)&main_scene.meta.enable_dof)) {
            main_scene.meta.sample_index = 1;
        }

        ImGui::SliderInt("max bounces", (int32_t *)&main_scene.meta.max_bounce, 1, 250);
        ImGui::SliderInt("min bounces", (int32_t *)&main_scene.meta.min_bounce, 1, main_scene.meta.max_bounce);
        if (ImGui::SliderInt("downscale factor", (int32_t *)&main_scene.meta.downscale_factor, 1, 32)) {
            main_scene.meta.sample_index = 1;
        }

        if (ImGui::Checkbox("debug bvh", (bool *)&main_scene.meta.debug_bvh)) { main_scene.meta.sample_index = 1; }

        ImGui::End();

        ImGui::EndFrame();

        // To start a frame capture, call StartFrameCapture.
        // You can specify NULL, NULL for the device to capture on if you have only one device and
        // either no windows at all or only one window, and it will capture from that device.
        // See the documentation below for a longer explanation
        // if(rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);

        //     renderer.ui();

        if (can_render) {
            ImGui::NewFrame();

            // ImGui::SetNextWindowPos({0.f, 0.f});
            // ImGui::SetNextWindowSize({0.f, 0.f});
            // ImGui::Begin("Debug", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

            // ImGui::Text("frame per second %u\n", static_cast<uint32_t>(1000.f / delta_time));
            // ImGui::Text("frame time %f ms\n", delta_time);
            // ImGui::Text("frame count %u\n", main_scene.meta.sample_index);

            // if (ImGui::Checkbox("depth of field", (bool*)&main_scene.meta.enable_dof)) {
            //     main_scene.meta.sample_index = 1;
            // }

            // ImGui::SliderInt("max bounces", (int32_t *)&main_scene.meta.max_bounce, 1, 250);
            // ImGui::SliderInt("min bounces", (int32_t *)&main_scene.meta.min_bounce, 1, main_scene.meta.max_bounce);
            // if (ImGui::SliderInt("downscale factor", (int32_t *)&main_scene.meta.downscale_factor, 1, 32)) {
            //     main_scene.meta.sample_index = 1;
            // }

            // if (ImGui::Checkbox("debug bvh", (bool *)&main_scene.meta.debug_bvh)) {
            //     main_scene.meta.sample_index = 1;
            // }

            // ImGui::End();
            bool open = true;
            ImGui:ImGui::ShowDemoWindow(&open);

            ImGui::EndFrame();

            ImGui::Render();

            // To start a frame capture, call StartFrameCapture.
            // You can specify NULL, NULL for the device to capture on if you have only
            // one device and either no windows at all or only one window, and it will
            // capture from that device. See the documentation below for a longer
            // explanation if(rdoc_api) rdoc_api->StartFrameCapture(NULL, NULL);

            renderer.update_buffer(main_scene.scene_buffer, &main_scene.meta, 0, sizeof(main_scene.meta));

            raytracing_pass->set_ouput_texture(output_texture);
            raytracing_pass->set_constant(0, main_scene.scene_buffer);
            raytracing_pass->set_constant(60, accumulation_texture);


            ImDrawData* draw_data = ImGui::GetDrawData();

            ui_pass->set_viewport(0.f, 0.f, (float)draw_data->DisplaySize.x, (float)draw_data->DisplaySize.y);

            auto previous_size = ui_primitives.size();
            auto commands_count = 0;

            for (uint32_t draw_list_index = 0; draw_list_index < draw_data->CmdListsCount; draw_list_index++) {
                commands_count += draw_data->CmdLists[draw_list_index]->CmdBuffer.size();
            }

            if (ui_primitives.size() < commands_count) {
                ui_primitives.resize(commands_count);

                for (size_t index = previous_size; index < ui_primitives.size(); index++) {
                    ui_primitives[index] = renderer.create_primitive(*ui_pass);
                    ui_primitives[index]->set_pipeline(ui_shader_name);
                    ui_primitives[index]->set_constant_offset(64);
                    ui_primitives[index]->set_constant(32, ui_texture);
                    ui_primitives[index]->set_constant(36, ui_texture_sampler);
                }
            }

            float scale[] {
                2.f / draw_data->DisplaySize.x,
                2.f / draw_data->DisplaySize.y,
            };
            float translate[] {
                -1.f - draw_data->DisplayPos.x * (2.f / draw_data->DisplaySize.x),
                -1.f - draw_data->DisplayPos.y * (2.f / draw_data->DisplaySize.y),
            };

            if (!ui_vertex_buffer || ui_vertex_buffer->size < draw_data->TotalVtxCount * sizeof(VkVertex)) {
                ui_vertex_buffer = renderer.create_buffer(draw_data->TotalVtxCount * sizeof(VkVertex), false);

                for (auto& primitive : ui_primitives) {
                    primitive->set_constant(0, ui_vertex_buffer);
                }
            }

            if (!ui_index_buffer || ui_index_buffer->size < draw_data->TotalIdxCount * sizeof(ImDrawIdx)) {
                ui_index_buffer = renderer.create_index_buffer(draw_data->TotalIdxCount * sizeof(ImDrawIdx), false);

                for (auto& primitive : ui_primitives) {
                    primitive->set_index_buffer(ui_index_buffer);
                }
            }

            off_t vertex_buffer_offset = 0;
            off_t index_buffer_offset = 0;
            std::vector<VkVertex> vertices;
            uint32_t draw_call_index = 0;
            for (uint32_t draw_list_index = 0; draw_list_index < draw_data->CmdListsCount; draw_list_index++) {
                auto& vertex_buffer = draw_data->CmdLists[draw_list_index]->VtxBuffer;
                auto& index_buffer = draw_data->CmdLists[draw_list_index]->IdxBuffer;
                vertices.resize(vertex_buffer.size());

                for (size_t vert_index = 0; vert_index < vertex_buffer.Size; vert_index++) {
                    vertices[vert_index] = *((VkVertex*)&vertex_buffer[vert_index]);
                }

                renderer.update_buffer(ui_vertex_buffer, vertices.data(), vertex_buffer_offset * sizeof(VkVertex), vertices.size() * sizeof(VkVertex));
                renderer.update_buffer(ui_index_buffer, index_buffer.Data, index_buffer_offset * sizeof(ImDrawIdx), index_buffer.size() * sizeof(ImDrawIdx));

                ImDrawList* draw_list = draw_data->CmdLists[draw_list_index];
                for (auto& draw_cmd: draw_list->CmdBuffer) {
                    auto& current_primitive = ui_primitives[draw_call_index];
                    current_primitive->set_constant(8, (uint64_t*)&scale);
                    current_primitive->set_constant(16, (uint64_t*)&translate);

                    ImVec2 pos = draw_data->DisplayPos;
                    current_primitive->set_scissor(
                        (int32_t)draw_cmd.ClipRect.x - pos.x,
                        (int32_t)draw_cmd.ClipRect.y - pos.y,
                        (uint32_t)draw_cmd.ClipRect.z - pos.x,
                        (uint32_t)draw_cmd.ClipRect.w - pos.y
                    );

                    current_primitive->set_draw_info(
                        draw_cmd.ElemCount,
                        index_buffer_offset + draw_cmd.IdxOffset,
                        vertex_buffer_offset + draw_cmd.VtxOffset
                    );

                    ui_pass->add_primitive(current_primitive);
                    draw_call_index++;
                }

                vertex_buffer_offset += vertex_buffer.Size;
                index_buffer_offset += index_buffer.Size;
            }

            renderer.begin_frame();

            ui_pass->set_color_attachment(0, renderer.back_buffer());

            renderer.render();
            renderer.finish_frame();

            main_scene.meta.sample_index++;

            std::swap(output_texture, accumulation_texture);
        }

        // stop the capture
        // if(rdoc_api) rdoc_api->EndFrameCapture(NULL, NULL);

        frame_count++;
    }

    //-------------------------
    // Image exporter
    //-------------------------
    // PPM format header
    // std::cout << "P3\n" << width << '\n' << height << "\n255" << std::endl;
    // auto output_image = renderer.output_image();
    // for (size_t index = 0; index < height * width; index++) {
    //     write_color(std::cout, output_image[index]);
    // }

    // std::cerr << "Done !" << std::endl;

    return 0;
}
