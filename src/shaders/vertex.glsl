#version 330 core

layout(location = 0) in vec2 aPos;
uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(aPos, 0.f, 1.f);
}
