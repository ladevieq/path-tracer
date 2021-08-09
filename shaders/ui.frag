#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) out vec4 o_Color;

layout(location = 0) in struct {
    vec2 uv;
    vec4 color;
} In;

layout(set = 0, binding = 0) uniform sampler samplers[];
layout(set = 0, binding = 1) uniform texture2D textures[];

layout(push_constant) uniform constants {
    layout(offset = 56) uint texture_index;
    uint sampler_index;
} consts;

void main() {
    o_Color = In.color * texture(sampler2D(textures[consts.texture_index], samplers[consts.sampler_index]), In.uv.xy);
}
