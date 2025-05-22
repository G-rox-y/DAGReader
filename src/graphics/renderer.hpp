#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "pch.hpp"
#include <glm/gtx/rotate_vector.hpp>

// this class is thread safe since elements will be added through one thread and displayed through the other
class Renderer {
private:
    // -- stuff for drawing triangles --

    GLuint mt_VA; // id of the triangles vertex array
    GLuint mt_VB; // id of the triangles vertex buffer
    GLuint mt_EB; // id of the triangles element buffer (for indexing triangle edges into triangles)

    std::vector<float> mt_ivmem; // temporary memory for storing vertices of index array-ed triangles
    std::vector<float> mt_vmem; // temporary memory for storing vertices of other triangles
    std::vector<int> mt_iimem; // temporary memory for storing the index array of the index array-ed vertices 

    // the size values that need to be saved cause they can change before drawing
    GLuint mt_indexSize, mt_indexedSize, mt_unindexedSize;
    
    // -- stuff for drawing lines --

    GLuint ml_VA; // id of the lines vertex array
    GLuint ml_VB; // id of the lines vertex buffer
    GLuint ml_EB; // id of the lines element buffer

    std::vector<float> ml_ivmem; // temporary memory for storing vertices of index array-ed lines
    std::vector<float> ml_vmem; // temporary memory for storing vertices of other lines
    std::vector<int> ml_iimem; // temporary memory for storing the index array of the index array-ed lines 

    // the size values that need to be saved cause they can change before drawing
    GLuint ml_indexSize, ml_indexedSize, ml_unindexedSize;

    // -- other stuff --

    std::mutex mem_mut; // this mutex guards memory

    std::atomic<bool> m_shouldUpdate{true};
public:

    Renderer();
    ~Renderer();

    void addQuad(glm::vec2 center, float length, float width, float angle);
    void addLine(const std::vector<float>& pts);

    void clearAll();

    // this function is going to fill the vertex buffer object with data and properly assign its vertex array
    void updateBuffers();

    void setShouldUpdate() { m_shouldUpdate.store(true); }

    void draw();
};