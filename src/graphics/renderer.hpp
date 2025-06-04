#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "pch.hpp"
#include <glm/gtx/rotate_vector.hpp>

// this class is thread safe since elements will be added through one thread and displayed through the other
class Renderer {
private:
    // -- bezier boxes --

    GLuint mbb_VA; // id of the bezier box vertex array
    GLuint mbb_VB; // id of the bezier box vertex buffer

    struct bezierBox {
        std::array<glm::vec4, 4> pt; // control poins
        glm::u8vec4 color;
        uint32_t _padding0; // padding so that we can pass the data directly and have it comply with glsl std430
        glm::vec2 halfExt; // dimensions of the box cross-section
        glm::vec2 _padding1; 
    };

    std::vector<bezierBox> mbb_mem; // temporary memory for storing vertices of index array-ed lines

    GLuint mbb_indexSize = 0; // the saved index size between updates

    // -- other stuff --

    std::mutex mem_mut; // this mutex guards memory

    std::atomic<bool> m_shouldUpdate{true};
public:

    Renderer();
    ~Renderer();

    void addBezierBox(glm::vec3 begin, glm::vec3 beginNormal, glm::vec3 end, glm::vec3 endNormal, glm::u8vec4 color);

    void clearAll();

    // this function is going to fill the vertex buffer object with data and properly assign its vertex array
    void updateBuffers();

    void setShouldUpdate() { m_shouldUpdate.store(true); }

    void draw();
};