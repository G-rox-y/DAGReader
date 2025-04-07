#include "quad.hpp"

void Quad::initBuffers()
{
    // generate the buffers and arrays
    glGenVertexArrays(1, &m_VA);
    glGenBuffers(1, &m_VB);
    glGenBuffers(1, &m_EB);
    
    // bind the vertex array
    glBindVertexArray(m_VA);

    // fill the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_VB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_pts), m_pts.data(), GL_DYNAMIC_DRAW);
    // TODO: the dynamic draw thats here may not need to be dynamic, for very rare update
    // you should judge this at some point later in development when you will know all use cases of this class

    // set the vertex layout
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // data for the index buffer (its ok for this to be hardcoded)
    std::array<unsigned int, 6> indices = {
        0, 1, 2, // first triangle is points 0, 1 and 2
        2, 3, 0 // second triangle is points 2, 3 and 0
    };

    // fill the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW); 

    m_didInit = true;
}

Quad::Quad(const std::array<float, 8>& pts) : drawable(false), m_pts(pts)
{}
Quad::Quad(glm::vec2 center, float length, float width, float angle) : drawable(false) // note: float angle is expected to be in radians
{
    // points of the quad
    glm::vec2 p1(center.x - length, center.y - width), p2(center.x + length, center.y - width),
        p3(center.x + length, center.y + width), p4(center.x - length, center.y + width);

    // now you should have them rotated
    glm::vec2 p1_rot = glm::rotate(p1 - center, angle) + center, p2_rot = glm::rotate(p2 - center, angle) + center,
        p3_rot = glm::rotate(p3 - center, angle) + center, p4_rot = glm::rotate(p4 - center, angle) + center;

    // and fill the array
    m_pts = {
        p1_rot.x, p1_rot.y,
        p2_rot.x, p2_rot.y,
        p3_rot.x, p3_rot.y,
        p4_rot.x, p4_rot.y
    };

    // and now init the buffers and the vertex array
}

Quad::~Quad()
{
    if (m_didInit){
        glDeleteBuffers(1, &m_VB);
        glDeleteBuffers(1, &m_EB);
        glDeleteVertexArrays(1, &m_VA);
    }
}

void Quad::draw() const
{
    glBindVertexArray(m_VA);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // 6 because i use 6 indices (hardcoded)
}