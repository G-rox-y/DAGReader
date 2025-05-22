#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "pch.hpp"
#include <glm/gtx/rotate_vector.hpp>

// this class is thread safe since elements will be added through one thread and displayed through the other
class Renderer {
private:
    GLuint m_VA; // id of the vertex array
    GLuint m_VB; // id of the vertex buffer
    GLuint m_EB; // id of the element buffer (for indexing triangle edges into triangles)

    std::vector<float> m_vmem; // temporary memory for storing vertices of other triangles
    std::vector<float> m_ivmem; // temporary memory for storing vertices of index array-ed triangles
    std::vector<int> m_iimem; // temporary memory for storing index array of the index array-ed vertices 

    // the values that need to be saved cause they can change before drawing
    GLuint m_indexSize, m_indexedSize, m_unindexedSize;

    std::mutex mem_mut; // this mutex guards memory

    std::atomic<bool> m_shouldUpdate{true};
public:

    Renderer();
    ~Renderer();

    void addQuad(glm::vec2 center, float length, float width, float angle);

    void clearAll();

    // this function is going to fill the vertex buffer object with data and properly assign its vertex array
    void updateBuffers();

    void setShouldUpdate() { m_shouldUpdate.store(true); }

    void draw();
};