layout(location = 0) out vec4 o_Color;

layout(location = 0) in struct {
    vec2 uv;
    vec4 color;
} In;

layout(set = 0, binding = 1) uniform sampler2D atlas;

void main() {
    o_Color = In.color * texture(atlas, In.uv.xy);
}
