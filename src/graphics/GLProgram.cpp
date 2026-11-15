#include "GLProgram.hpp"

GLuint GLProgram::loadShader(ShaderType type) {
    const char* code = nullptr;
    GLuint GLshaderType;
    switch (type) {
        case VERT_SHADER: code = vertexShaderSource; GLshaderType = GL_VERTEX_SHADER; break;
        case FRAG_SHADER: code = fragmentShaderSource; GLshaderType = GL_FRAGMENT_SHADER; break;
        case BEZIER_SHADER: code = computeShaderBezierSource; GLshaderType = GL_COMPUTE_SHADER; break;
        default: spdlog::warn("Shader type '{}' not allowed", static_cast<int>(type)); break;
    }
    GLuint s = glCreateShader(GLshaderType);
    glShaderSource(s, 1, &code, nullptr);
    glCompileShader(s);

    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        std::array<char, 512> log;
        glGetShaderInfoLog(s, log.size(), nullptr, log.data());
        spdlog::error("Shader fail: {}", log.data());
    }
    return s;
}


GLProgram::GLProgram(std::initializer_list<ShaderType> stages) {
    id = glCreateProgram();
    std::vector<GLuint> attached;
    for (auto t : stages) {
        GLuint s = loadShader(t);
        if (s) { glAttachShader(id, s); attached.push_back(s); }
    }
    glLinkProgram(id);
    GLint ok; glGetProgramiv(id, GL_LINK_STATUS, &ok);
    if (!ok) {
        std::array<char, 512> log;
        glGetProgramInfoLog(id, log.size(), nullptr, log.data());
        spdlog::error("Link fail: {}", log.data());
    }
    for (auto s : attached) glDeleteShader(s);
    use();
}

GLProgram::~GLProgram(){
    glDeleteProgram(id);
}


GLint GLProgram::getUniformLocation(const std::string& param)
{
    auto it = uniformLocationCache.find(param);
    if (it != uniformLocationCache.end()) return it->second;
    GLint loc = glGetUniformLocation(id, param.c_str());
    if (loc == -1) spdlog::warn("Uniform '{}' missing", param);
    uniformLocationCache[param] = loc;
    return loc;
}

void GLProgram::setUniformMat4f(const std::string& param, const glm::mat4& matrix){
    use();
    glUniformMatrix4fv(this->getUniformLocation(param), 1, false, &matrix[0][0]);
}

void GLProgram::setUniformVec3f(const std::string& param, const glm::vec3& vector){
    use();
    glUniform3fv(this->getUniformLocation(param), 1, &vector[0]);
}

void GLProgram::setUniform1f(const std::string& param, float num){
    use();
    glUniform1f(this->getUniformLocation(param), num);
}

void GLProgram::setUniform1i(const std::string& param, int num){
    use();
    glUniform1i(this->getUniformLocation(param), num);
}
