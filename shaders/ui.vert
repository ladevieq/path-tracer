struct Vertex {
    vec2    pos;
    vec2    uv;
    uint    color;
};

layout(set = 0, binding = 0) uniform Vertices {
    Vertex vertices[158];
};

layout(set = 0, binding = 1) uniform UiTransforms {
    vec2 scale;
    vec2 translate;
};

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out struct {
    vec2 uv;
    vec4 color;
} Out;


void main() {
    Vertex v = vertices[gl_VertexIndex];

    Out.color = unpackUnorm4x8(v.color);
    Out.uv = v.uv;

    gl_Position = vec4(v.pos * scale + translate, 0.0, 1.0);
}
