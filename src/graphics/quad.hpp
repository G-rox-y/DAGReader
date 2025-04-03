#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <array>

#include "drawable.hpp"

class Quad : public drawable{
private:
    std::array<float, 8> m_pts; // xy coordinates of 4 points that describe it

    GLuint m_VB; // id of the vertex buffer
    GLuint m_VA; // id of the vertex array
    GLuint m_EB; // id of the element buffer (for indexing triangle edges into triangles)

    // this function is going to fill the vertex buffer object with data and properly assign its vertex array
    void initBuffers();
public:
    // standard constructor
    Quad(const std::array<float, 8>& pts);
    Quad(glm::vec2 center, float length, float width, float angle);
    ~Quad();

    void draw() override;
};