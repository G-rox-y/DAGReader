#pragma once

extern const char* vertexShaderSource;
extern const char* fragmentShaderSource;

// thats all for this file, now you may wonder where are these ShaderSource-s defined?
// the file in this folder called shaderEmbed.cmake will upon building the project create respective ShaderSource.cpp files
// the file will contain respective definitions which will be pulled directly from the .glsl file
// this may seem overcomplicated but i wanted to preserve glsl linting without errors
