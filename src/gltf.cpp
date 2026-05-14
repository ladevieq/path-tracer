#include "gltf.hpp"

using namespace simdjson;

ondemand::object root;

struct accessor {
    u32 view_index  = 0U;
    u32 count       = 0U;
    u32 component_type = 0U;
    std::string_view type;
};

struct buffer_view {
    u32 buffer_index    = 0U;
    u32 length          = 0U;
    u32 offset          = 0U;
};

// component types
#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_FLOAT                          0x1406
#define GL_FIXED                          0x140C

#define SCALAR  0x5343414C4152
#define VEC2    0x56454332
#define VEC3    0x56454333
#define VEC4    0x56454334

static u32 accessor_type_to_component_count(std::string_view& type) {
    if (strncmp("SCALAR", type.data(), type.size()) == 0) {
        return 1U;
    } else if (strncmp("VEC2", type.data(), type.size()) == 0) {
        return 2U;
    } else if (strncmp("VEC3", type.data(), type.size()) == 0) {
        return 3U;
    } else if (strncmp("VEC4", type.data(), type.size()) == 0) {
        return 4U;
    }
    assert(false);
    return 0U;
}

static u32 component_type_size(u32 type) {
    switch (type) {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
            return 1U;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
            return 2U;
        case GL_FLOAT:
        case GL_FIXED:
            return 4U;
        default: assert(false);
    }
    return 0U;
}

static Mesh::submesh load_primitive(ondemand::object& primitive, std::vector<struct buffer>& buffers, std::vector<struct accessor>& accessors, std::vector<struct buffer_view>& buffer_views) {
    Mesh::submesh submesh;
    
    // indices
    {
        u32 indices_id = primitive["indices"].get_uint32();
        struct accessor& accessor = accessors[indices_id];

        struct buffer_view& view = buffer_views[accessor.view_index];
        struct buffer& buffer = buffers[view.buffer_index];

        submesh.indices = {
            .data = &buffer.bytes[view.offset],
            .length = accessor.count * accessor_type_to_component_count(accessor.type) * component_type_size(accessor.component_type),
        };

        submesh.index_count = accessor.count;
    }

    // positions
    {
        u32 pos_id = primitive["attributes"]["POSITION"].get_uint32();
        struct accessor& accessor = accessors[pos_id];

        struct buffer_view& view = buffer_views[accessor.view_index];
        struct buffer& buffer = buffers[view.buffer_index];

        submesh.attributes[ATTRIBUTE_TYPE::POSITION]  = {
            .data = &buffer.bytes[view.offset],
            .length = accessor.count * accessor_type_to_component_count(accessor.type) * component_type_size(accessor.component_type),
        };

        submesh.vertex_count = accessor.count;
    }

    u32 normal_index = 0U;
    if (!primitive["attributes"]["NORMAL"].get(normal_index)) {
        struct accessor& accessor = accessors[normal_index];
        u32 view_index = accessor.view_index;

        struct buffer_view& view = buffer_views[view_index];
        struct buffer& buffer = buffers[view.buffer_index];

        submesh.attributes[ATTRIBUTE_TYPE::NORMAL] = {
            .data = &buffer.bytes[view.offset],
            .length = accessor.count * accessor_type_to_component_count(accessor.type) * component_type_size(accessor.component_type),
        };
    }

    u32 texcoord_index = 0U;
    if (!primitive["attributes"]["TEXCOORD_0"].get(texcoord_index)) {
        struct accessor& accessor = accessors[texcoord_index];
        u32 view_index = accessor.view_index;

        struct buffer_view& view = buffer_views[view_index];
        struct buffer& buffer = buffers[view.buffer_index];

        submesh.attributes[ATTRIBUTE_TYPE::UV_0] = {
            .data = &buffer.bytes[view.offset],
            .length = accessor.count * accessor_type_to_component_count(accessor.type) * component_type_size(accessor.component_type),
        };
    }

    return submesh;
}

static void load_textures(char* path, std::vector<handle<device_texture>>& textures, std::vector<struct buffer>& buffers, std::vector<struct buffer_view>& buffer_views, bool is_glb) {
    ondemand::array gltf_images = root["images"];
    std::vector<raw_image> images(gltf_images.count_elements());
    size_t path_len = strlen(path);
    char path_buffer[2048] = { 0U };
    strncpy(path_buffer, path, path_len);
    BEGIN_MARKER(LOAD_TEXTURES);
    u32 image_index = 0U;
    for (ondemand::object gltf_image : gltf_images) {
        if (is_glb) {
            // TODO: consider mimeType
            struct buffer_view& view = buffer_views[gltf_image["bufferView"]];
            struct buffer& buffer = buffers[view.buffer_index];

            raw_image& image = images[image_index++];

            image.data = stbi_load_from_memory(&buffer.bytes[view.offset], view.length, &image.width, &image.height, &image.channels, 4);

        } else {
            std::string_view uri_view = gltf_image["uri"];
            strncpy(path_buffer + path_len, uri_view.data(), uri_view.length());
            path_buffer[path_len + uri_view.length()] = '\0';

            raw_image& image = images[image_index++];
            image.data = stbi_load(path_buffer, &image.width, &image.height, &image.channels, 4);
        }
    }
    END_MARKER(LOAD_TEXTURES);

    // ondemand::array gltf_samplers = root["samplers"];
    // std::vector<handle<device_sampler>> samplers(gltf_samplers.count_elements());

    // auto sampler = device.create_sampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    // for (size_t sampler_index = 0; sampler_index < samplers_count; sampler_index++) {
    //     samplers[sampler_index] = vkrenderer::create_sampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    // }

    vkdevice& device = vkdevice::get();

    constexpr auto buffers_count = 2U;
    size_t total_size = images.size() * 4096U * 4096 * sizeof(uint32_t);
    // TODO: Fix nasty stuff going on
    // Crash on buffer destruction and load wrong images
    // size_t buffer_size = total_size / 32U;
    size_t buffer_size = total_size / 4U;

    handle<device_buffer> staging_buffers_handle[buffers_count] = {
        device.create_buffer({
            .size = buffer_size,
            .usages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        }),
        device.create_buffer({
            .size = buffer_size,
            .usages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        }),
    };

    // TODO: Create a proper upload heap
    BEGIN_MARKER(UPLOAD_TEXTURES);
    graphics_command_buffer command_buffers[buffers_count];
    device.allocate_command_buffers(command_buffers, buffers_count, QueueType::GRAPHICS, "upload_buffer");
    auto buffer_index = 0U;
    auto command_buffer = command_buffers[buffer_index];
    auto buffer_handle = staging_buffers_handle[buffer_index];
    device_buffer& buffer = device.get_buffer(buffer_handle);

    command_buffer.start();

    ondemand::array gltf_textures = root["textures"];
    textures.resize(gltf_textures.count_elements());
    u32 texture_index = 0U;
    for (ondemand::object gltf_texture: gltf_textures) {
        raw_image& image = images[gltf_texture["source"]];
        // if (gltf_texture.contains("sampler")) {
        //     u32 sampler_index = gltf_texture["sampler"].get_uint32();
        //     sampler = samplers[sampler_index];
        // }

        handle<device_texture> texture = device.create_texture({
            .width  = static_cast<uint32_t>(image.width),
            .height = static_cast<uint32_t>(image.height),
            .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .type   = VK_IMAGE_TYPE_2D,
        });

        u32 size = image.width * image.height * sizeof(uint32_t);
        if (buffer.allocated_size + size > buffer.size) {
            command_buffer.stop();

            device.submit(&command_buffer, 1);

            buffer_index = (buffer_index + 1U) % buffers_count;
            command_buffer = command_buffers[buffer_index];
            buffer_handle = staging_buffers_handle[buffer_index];
            buffer = device.get_buffer(buffer_handle);
            buffer.reset();
            // TODO: Remove the wait
            // device.wait();
            command_buffer.start();
        }

        memcpy(buffer.allocate(size), image.data, size);

        command_buffer.barrier(texture, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        command_buffer.copy(buffer_handle, texture, static_cast<VkDeviceSize>(buffer.offset));
        command_buffer.barrier(texture, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

        textures[texture_index++] = texture;
    }

    if (buffer.allocated_size > 0U) {
        command_buffer.stop();
        device.submit(&command_buffer, 1);
    }
    device.wait();
    device.destroy_buffer(staging_buffers_handle[0]);
    device.destroy_buffer(staging_buffers_handle[1]);
    END_MARKER(UPLOAD_TEXTURES);
}

static void load_materials(std::vector<material>& materials, std::vector<handle<device_texture>>& textures) {
    ondemand::array gltf_materials = root["materials"];
    materials.resize(gltf_materials.count_elements());

    u32 material_index = 0U;
    for (ondemand::object object: gltf_materials) {
        material& material = materials[material_index++];
        material.base_color = vec3(1.f);
        material.metalness = 1.f;
        material.roughness = 1.f;

        ondemand::object pbr_params;
        if (!object["pbrMetallicRoughness"].get(pbr_params)) {
            ondemand::array base_color;
            if (!pbr_params["baseColorFactor"].get(base_color)) {
                u32 id = 0U;
                for (double val : base_color) {
                    material.base_color.arr[id++] = static_cast<float>(val);
                }
            }

            ondemand::object base_color_texture;
            if (!pbr_params["baseColorTexture"].get(base_color_texture)) {
                material.base_color_texture = textures[base_color_texture["index"]];
            }

            pbr_params["metallicFactor"].get(material.metalness);
            pbr_params["roughnessFactor"].get(material.roughness);

            ondemand::object metallic_roughness_texture;
            if (!pbr_params["metallicRoughnessTexture"].get(metallic_roughness_texture)) {
                material.metallic_roughness_texture = textures[metallic_roughness_texture["index"]];
            }
        }
    }
}

gltf::gltf(const char* filepath, bool is_glb) {
    BEGIN_MARKER(LOAD_GLTF);

    ondemand::parser parser;
    padded_string json;
    ondemand::document doc;

    char buffer_path[2048U] = { 0U };
    char* filename = nullptr;
    GetFullPathNameA( filepath, 2048U, buffer_path, &filename);
    size_t parent_dir_len = filename - buffer_path;

    if (is_glb) {
        size_t filesize = 0U;
        size_t read = 0U;
        u8* glb_buffer = read_file(filepath, &filesize);
        header =  (struct glb_header*)glb_buffer;
        assert(filesize == header->length);
        // assert(header->magic == (u32)"glTF");

        read += sizeof(*header);
        chunks.push_back((struct glb_chunk*)(header + 1));
        read += offsetof(struct glb_chunk, data) + chunks.back()->length;
        while (read != header->length) {
            chunks.push_back((struct glb_chunk*)(glb_buffer + read));
            read += offsetof(struct glb_chunk, data) + chunks.back()->length;
        }

        doc = parser.iterate((const char*)&chunks[0]->data[0U], chunks[0]->length, filesize);
        root = doc;

        ondemand::array gltf_buffers = root["buffers"];
        assert(gltf_buffers.count_elements() == chunks.size() - 1U);
        buffers.resize(gltf_buffers.count_elements());
        u32 buffer_index = 0U;
        for (u32 chunk_index = 1U; chunk_index < chunks.size(); chunk_index++) {
            buffers[buffer_index++] = buffer {
                .bytes = chunks[chunk_index]->data,
                .size = chunks[chunk_index]->length,
            };
        }
    } else {
        json = padded_string::load(filepath);
        doc = parser.iterate(json);
        root = doc;

        {
            ondemand::array gltf_buffers = root["buffers"];
            buffers.resize(gltf_buffers.count_elements());
            u32 buffer_index = 0U;
            for (ondemand::object value: gltf_buffers) {
                std::string_view uri_view = value["uri"];
                strncpy(buffer_path + parent_dir_len, uri_view.data(), uri_view.length());
                buffer_path[parent_dir_len + uri_view.length()] = '\0';

                size_t file_size = 0U;
                uint8_t* bytes = read_file(buffer_path, &file_size);
                buffers[buffer_index++] = {
                    .bytes = bytes,
                    .size = file_size,
                };
            }
        }
    }


    std::vector<struct accessor> accessors;
    {
        ondemand::array gltf_accessors = root["accessors"];
        accessors.resize(gltf_accessors.count_elements());
        u32 accessor_index = 0U;
        for (ondemand::object value: gltf_accessors) {
            struct accessor& accessor = accessors[accessor_index++];
            value["bufferView"].get(accessor.view_index);
            value["count"].get(accessor.count);
            value["type"].get(accessor.type);
            value["componentType"].get(accessor.component_type);
        }
    }

    std::vector<struct buffer_view> buffer_views;
    {
        ondemand::array gltf_views = root["bufferViews"];
        buffer_views.resize(gltf_views.count_elements());
        u32 view_index = 0U;
        for (ondemand::object value: gltf_views) {
            struct buffer_view& view = buffer_views[view_index++];
            value["buffer"].get(view.buffer_index);
            value["byteLength"].get(view.length);
            value["byteOffset"].get(view.offset);
        }
    }


    {
        ondemand::array gltf_nodes = root["nodes"];
        nodes.resize(gltf_nodes.count_elements());

        u32 node_index = 0U;
        for (ondemand::object gltf_node: gltf_nodes) {
            struct node& node = nodes[node_index++];

            ondemand::array gltf_children;
            if (!gltf_node["children"].get(gltf_children)) {
                node.children.resize(gltf_children.count_elements());

                u32 child_index = 0U;
                for (auto child : gltf_children) {
                    node.children[child_index++] = child.get_uint32();
                }
            }

            node.mesh = UINT32_MAX;
            gltf_node["mesh"].get(node.mesh);
        }
    }


    // root nodes
    {
        u32 scene_index = root["scene"].get_uint32();
        ondemand::array gltf_scene_nodes = root["scenes"].at(scene_index)["nodes"];
        root_nodes.resize(gltf_scene_nodes.count_elements());

        u32 index = 0U;
        for (auto node_index: gltf_scene_nodes) {
            root_nodes[index++] = node_index.get_uint32();
        }
    }

    buffer_path[parent_dir_len] = '\0';
    load_textures(buffer_path, textures, buffers, buffer_views, is_glb);
    load_materials(materials, textures);

    // meshes
    {
        ondemand::array gltf_meshes = root["meshes"];
        meshes.resize(gltf_meshes.count_elements());

        u32 mesh_index = 0U;
        for (ondemand::object gltf_mesh: gltf_meshes) {
            Mesh& mesh = meshes[mesh_index++];
            ondemand::array gltf_primitives = gltf_mesh["primitives"];

            for (ondemand::object primitive : gltf_primitives) {
                material& material = materials[primitive["material"]];
                mesh.add_submesh(load_primitive(primitive, buffers, accessors, buffer_views), material);
            }

            Mesh::register_mesh(mesh);
        }
    }

    END_MARKER(LOAD_GLTF);
}

void gltf::deinit() {
    for (handle<device_texture> handle: textures) {
        vkdevice::get().destroy_texture(handle);
    }

    for (struct buffer& buffer: buffers) {
        free(buffer.bytes);
    }

    free(header);
}
