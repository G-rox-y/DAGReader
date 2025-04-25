#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "pch.hpp"
#include <glm/gtx/rotate_vector.hpp>

class Quad : public drawable{
private:
    std::array<float, 8> m_pts; // xy coordinates of 4 points that describe it

    GLuint m_VB; // id of the vertex buffer
    GLuint m_VA; // id of the vertex array
    GLuint m_EB; // id of the element buffer (for indexing triangle edges into triangles)

public:
    Quad(const std::array<float, 8>& pts);
    Quad(glm::vec2 center, float length, float width, float angle);
    ~Quad();

    // this function is going to fill the vertex buffer object with data and properly assign its vertex array
    void initBuffers() override;

    void draw() const override;
};