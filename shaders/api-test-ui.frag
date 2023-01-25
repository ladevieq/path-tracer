#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) out vec4 o_Color;

layout(location = 0) in struct {
    vec2 uv;
    vec4 color;
} In;

layout(set = 0, binding = 0) uniform sampler samplers[];
layout(set = 0, binding = 1) uniform texture2D textures[];
layout(set = 0, binding = 2, rgba32f) uniform image2D images[];
layout(set = 1, binding = 0) uniform parameters {
    uint texture_index;
    uint sampler_index;
} params;

void main() {
    o_Color = In.color * texture(sampler2D(textures[params.texture_index], samplers[params.sampler_index]), In.uv.xy);
}
