#include "renderer.hpp"


Renderer::Renderer()
{
    glGenVertexArrays(1, &mt_VA);
    glGenBuffers(1, &mt_VB);
    glGenBuffers(1, &mt_EB);

    glGenVertexArrays(1, &ml_VA);
    glGenBuffers(1, &ml_VB);
    glGenBuffers(1, &ml_EB);
}

Renderer::~Renderer()
{
    glDeleteBuffers(1, &ml_EB);
    glDeleteBuffers(1, &ml_VB);
    glDeleteVertexArrays(1, &ml_VA);
    
    glDeleteBuffers(1, &mt_EB);
    glDeleteBuffers(1, &mt_VB);
    glDeleteVertexArrays(1, &mt_VA);
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
    int b = mt_ivmem.size() / 2;
    mt_iimem.insert(mt_iimem.end(), {
        b+0, b+1, b+2,
        b+2, b+3, b+0
    });

    // and fill the array
    mt_ivmem.insert(mt_ivmem.end(), {
        p1_rot.x, p1_rot.y,
        p2_rot.x, p2_rot.y,
        p3_rot.x, p3_rot.y,
        p4_rot.x, p4_rot.y
    });
}

void Renderer::addLine(const std::vector<float>& pts)
{
    std::lock_guard lk(mem_mut);

    if (pts.size() < 4) return;
    else if (pts.size() == 4){
        ml_vmem.insert(ml_vmem.end(), pts.begin(), pts.end());
        return;
    }

    int b = ml_ivmem.size() / 2;
    for(size_t i = 1; i < pts.size()/2; i++)
        ml_iimem.insert(ml_iimem.begin(), {b+(int)i-1, b+(int)i});
    
    ml_ivmem.insert(ml_ivmem.end(), pts.begin(), pts.end());
}

void Renderer::clearAll()
{
    std::lock_guard lk(mem_mut);

    mt_ivmem.clear(); mt_vmem.clear(); mt_iimem.clear();
    mt_indexedSize = mt_unindexedSize = mt_indexSize = 0;

    ml_ivmem.clear(); ml_vmem.clear(); ml_iimem.clear();
    ml_indexedSize = ml_unindexedSize = ml_indexSize = 0;

    m_shouldUpdate.store(true);
}

void Renderer::updateBuffers()
{
    if (!m_shouldUpdate.load()) return;
    
    std::lock_guard lk(mem_mut);

    // bind the vertex array
    glBindVertexArray(mt_VA);

    // combine vertex data
    std::vector<float> combined;
    combined.reserve(mt_ivmem.size() + mt_vmem.size());
    combined.insert(combined.end(), mt_ivmem.begin(), mt_ivmem.end());
    combined.insert(combined.end(), mt_vmem.begin(), mt_vmem.end());

    // fill the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, mt_VB);
    glBufferData(GL_ARRAY_BUFFER, combined.size() * sizeof(float), combined.data(), GL_DYNAMIC_DRAW);
    // TODO: the dynamic draw thats here may not need to be dynamic, for very rare update
    // you should judge this at some point later in development when you will know all use cases of this class

    // set the vertex layout
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // fill the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mt_EB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mt_iimem.size() * sizeof(int), mt_iimem.data(), GL_STATIC_DRAW); 

    m_shouldUpdate.store(false);

    mt_indexSize = mt_iimem.size();
    mt_indexedSize = mt_ivmem.size() / 2;
    mt_unindexedSize = mt_vmem.size() / 2;

    // now the lines
    glBindVertexArray(ml_VA);

    combined.clear();
    combined.reserve(ml_ivmem.size() + ml_vmem.size());
    combined.insert(combined.end(), ml_ivmem.begin(), ml_ivmem.end());
    combined.insert(combined.end(), ml_vmem.begin(), ml_vmem.end());

    glBindBuffer(GL_ARRAY_BUFFER, ml_VB);
    glBufferData(GL_ARRAY_BUFFER, combined.size() * sizeof(float), combined.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ml_EB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ml_iimem.size() * sizeof(int), ml_iimem.data(), GL_STATIC_DRAW); 

    m_shouldUpdate.store(false);

    ml_indexSize = ml_iimem.size();
    ml_indexedSize = ml_ivmem.size() / 2;
    ml_unindexedSize = ml_vmem.size() / 2;
}

void Renderer::draw()
{
    updateBuffers();
    glBindVertexArray(mt_VA);
    glDrawElements(GL_TRIANGLES, mt_indexSize, GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, mt_indexedSize, mt_unindexedSize);

    glBindVertexArray(ml_VA);
    glDrawElements(GL_LINES, ml_indexSize, GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_LINES, ml_indexedSize, ml_unindexedSize);
}