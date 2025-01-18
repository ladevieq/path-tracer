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
// layout(set = 2, binding = 0) uniform parameters {
//     uint vertex_buffer;
//     vec2 scale;
//     vec2 translate;
//     uint texture_index;
// } params;

layout(buffer_reference) readonly buffer params {
    vec2 scale;
    vec2 translate;
    uint texture_index;
    uint padding;
};

layout(push_constant) uniform constants {
    layout(offset = 16) params ui_params;
} consts;

void main() {
    o_Color = In.color * texture(sampler2D(textures[nonuniformEXT(consts.ui_params.texture_index)], samplers[nonuniformEXT(0U)]), In.uv.xy);
}
