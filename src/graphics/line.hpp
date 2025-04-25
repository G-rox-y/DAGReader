#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "pch.hpp"


class Line : public drawable{
private:
    std::vector<glm::vec2> m_pts; // lines have at least 2 points (1 segment), but can have more

    GLuint m_VB; // id of the vertex buffer
    GLuint m_VA; // id of the vertex array
public:
    Line(const std::vector<glm::vec2>& pts);
    ~Line();

    // this function is going to fill the vertex buffer object with data and properly assign its vertex array
    void initBuffers() override;

    void draw() const override;
};
