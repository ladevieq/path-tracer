#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) out vec4 o_Color;

layout(location = 0) in struct {
    vec2 uv;
    vec4 color;
} In;

layout(set = 0, binding = 0) uniform sampler samplers[];
layout(set = 0, binding = 1) uniform texture2D textures[];
layout(set = 0, binding = 2, rgba32f) uniform image2D images[];

layout(buffer_reference) readonly buffer global_params {
    uint padding;
};

layout(buffer_reference) readonly buffer pass_params {
    uint padding;
};

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

layout(buffer_reference) readonly buffer draw_params {
    vertices vertex_buffer;
    vec2 scale;
    vec2 translate;
    uint texture_index;
};

layout(push_constant) uniform registers {
    global_params globals;
    pass_params pass;
    draw_params draw;
} regs;

void main() {
    o_Color = In.color * texture(sampler2D(textures[nonuniformEXT(regs.draw.texture_index)], samplers[nonuniformEXT(0U)]), In.uv.xy);
}
