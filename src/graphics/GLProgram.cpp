#include "GLProgram.hpp"

GLuint GLProgram::loadShader(const GLuint type)
{   
    const char* code;
    
    if (type == GL_VERTEX_SHADER) code = vertexShaderSource;
    else if (type == GL_FRAGMENT_SHADER) code = fragmentShaderSource;
    else spdlog::warn("Shader type '{}' is not allowed", type);

    GLuint shader = glCreateShader(type);
    if (shader == GL_INVALID_ENUM)
        spdlog::warn("Shader type '{}' is invalid", type);

    glShaderSource(shader, 1, &code, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success){
        std::array<char, 512> infolog;
        glGetShaderInfoLog(shader, 512, NULL, infolog.data());
        spdlog::error("Shader fail: {}", infolog.data());
    }

    return shader;
}

GLProgram::GLProgram()
{
    id = glCreateProgram();
    
    GLuint vert = loadShader(GL_VERTEX_SHADER);
    glAttachShader(id, vert);
    
    GLuint frag = loadShader(GL_FRAGMENT_SHADER);
    glAttachShader(id, frag);

    glLinkProgram(id);

    GLint success;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success){
        std::array<char, 512> infolog;
        glGetProgramInfoLog(id, 512, NULL, infolog.data());
        spdlog::error("Shader fail: {}", infolog.data());
    }

    // we dont need shaders after compilation
    glDeleteShader(vert);
    glDeleteShader(frag);

    this->use();
}

GLProgram::~GLProgram()
{
    glDeleteProgram(id);
    this->programUnbind();
}


int GLProgram::getUniformLocation(const std::string& param)
{
    if (uniformLocationCache.find(param) != uniformLocationCache.end()) return uniformLocationCache[param];

    int location = glGetUniformLocation(id, param.c_str());
    
    if (location == -1) spdlog::warn("uniform {} doesnt exist", param);
    
    uniformLocationCache[param] = location;
    return location;
}


void GLProgram::setUniform3f(const std::string& param, float f1, float f2, float f3)
{
    this->use();
    glUniform3f(this->getUniformLocation(param), f1, f2, f3);
}

void GLProgram::setUniformMat4f(const std::string& param, const glm::mat4& matrix)
{
    this->use();
    glUniformMatrix4fv(this->getUniformLocation(param), 1, false, &matrix[0][0]);
}

void GLProgram::use() const
{
    glUseProgram(id); 
}