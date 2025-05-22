#include "renderer.hpp"


Renderer::Renderer()
{
    glGenVertexArrays(1, &m_VA);
    glGenBuffers(1, &m_VB);
    glGenBuffers(1, &m_VB);
    glGenBuffers(1, &m_EB);
}

Renderer::~Renderer()
{
    glDeleteBuffers(1, &m_EB);
    glDeleteBuffers(1, &m_VB);
    glDeleteVertexArrays(1, &m_VA);
}

void Renderer::addQuad(glm::vec2 center, float length, float width, float angle)
{
    // points of the quad
    glm::vec2 p1(center.x - length, center.y - width), p2(center.x + length, center.y - width),
        p3(center.x + length, center.y + width), p4(center.x - length, center.y + width);

    // now you should have them rotated
    glm::vec2 p1_rot = glm::rotate(p1 - center, angle) + center, p2_rot = glm::rotate(p2 - center, angle) + center,
        p3_rot = glm::rotate(p3 - center, angle) + center, p4_rot = glm::rotate(p4 - center, angle) + center;

    std::lock_guard lk(mem_mut);
    
    // fill the index array
    int b = m_ivmem.size() / 2;
    m_iimem.insert(m_iimem.end(), {
        b+0, b+1, b+2,
        b+2, b+3, b+0
    });

    // and fill the array
    m_ivmem.insert(m_ivmem.end(), {
        p1_rot.x, p1_rot.y,
        p2_rot.x, p2_rot.y,
        p3_rot.x, p3_rot.y,
        p4_rot.x, p4_rot.y
    });
}

void Renderer::clearAll()
{
    std::lock_guard lk(mem_mut);
    m_ivmem.clear(); m_vmem.clear(); m_iimem.clear();
    m_indexedSize = m_unindexedSize = m_indexSize = 0;
    m_shouldUpdate.store(true);
}

void Renderer::updateBuffers()
{
    if (!m_shouldUpdate.load()) return;
    
    std::lock_guard lk(mem_mut);

    // bind the vertex array
    glBindVertexArray(m_VA);

    // combine vertex data
    std::vector<float> combined;
    combined.reserve(m_ivmem.size() + m_vmem.size());
    combined.insert(combined.end(), m_ivmem.begin(), m_ivmem.end());
    combined.insert(combined.end(), m_vmem.begin(), m_vmem.end());

    // fill the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_VB);
    glBufferData(GL_ARRAY_BUFFER, combined.size() * sizeof(float), combined.data(), GL_DYNAMIC_DRAW);
    // TODO: the dynamic draw thats here may not need to be dynamic, for very rare update
    // you should judge this at some point later in development when you will know all use cases of this class

    // set the vertex layout
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // fill the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_iimem.size() * sizeof(int), m_iimem.data(), GL_STATIC_DRAW); 

    m_shouldUpdate.store(false);

    m_indexSize = m_iimem.size();
    m_indexedSize = m_ivmem.size() / 2;
    m_unindexedSize = m_vmem.size() / 2;
}

void Renderer::draw()
{
    updateBuffers();
    glBindVertexArray(m_VA);
    glDrawElements(GL_TRIANGLES, m_indexSize, GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, m_indexedSize, m_unindexedSize);
}