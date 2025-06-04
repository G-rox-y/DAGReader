#version 430 core

flat in vec4 gsColor;
out vec4 FragColor;

void main() {
    FragColor = gsColor;
}
