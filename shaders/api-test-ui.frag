#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_descriptor_heap : require

layout(location = 0) out vec4 o_Color;

layout(location = 0) in struct {
    vec2 uv;
    vec4 color;
} In;

layout(descriptor_heap) uniform texture2D textures[];
layout(descriptor_heap, rgba32f) uniform image2D images[];
layout(descriptor_heap) uniform sampler samplers[];

struct vertex {
    float    pos_x;
    float    pos_y;
    float    uv_x;
    float    uv_y;
    uint    color;
};

layout(buffer_reference) readonly buffer vertices {
    vertex vertices[];
};

layout(push_constant) uniform registers {
    vertices vertex_buffer;
    // vec2 scale;
    // vec2 translate;
    // uint texture_index;
    // uint padding;
};

void main() {
    // o_Color = In.color * texture(sampler2D(textures[nonuniformEXT(texture_index)], samplers[nonuniformEXT(0U)]), In.uv.xy);
    o_Color = vec4(1.f, 0.f, 0.f, 1.f);
}
