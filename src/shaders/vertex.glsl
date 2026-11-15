#version 430 core

struct VertexOut { vec4 pos; vec4 color; };
layout(std430, binding = 3) readonly buffer MeshOutput { VertexOut verts[]; };

flat out vec4 vColor;

void main() {
    VertexOut v = verts[gl_VertexID];
    gl_Position = v.pos;
    vColor = v.color;
}