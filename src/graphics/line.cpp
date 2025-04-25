#include "line.hpp"

Line::Line(const std::vector<glm::vec2>& pts) : drawable(false), m_pts(pts)
{}
Line::~Line()
{
    if (m_didInit){
        glDeleteBuffers(1, &m_VB);
        glDeleteVertexArrays(1, &m_VA);
    }
}

void Line::initBuffers()
{
    // generate the buffers and arrays
    glGenVertexArrays(1, &m_VA);
    glGenBuffers(1, &m_VB);
    
    // bind the vertex array
    glBindVertexArray(m_VA);

    // fill the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_VB);
    glBufferData(GL_ARRAY_BUFFER, m_pts.size() * sizeof(glm::vec2), m_pts.data(), GL_DYNAMIC_DRAW);
    // TODO: the dynamic draw thats here may not need to be dynamic, for very rare update
    // you should judge this at some point later in development when you will know all use cases of this class

    // set the vertex layout
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    m_didInit = true;
}

void Line::draw() const
{
    glBindVertexArray(m_VA);
    glDrawArrays(GL_LINE_STRIP, 0, m_pts.size());
}