#include "renderer.hpp"


Renderer::Renderer()
{
    glGenVertexArrays(1, &mbb_VA);
    glGenBuffers(1, &mbb_VB);
}

Renderer::~Renderer()
{
    glDeleteBuffers(1, &mbb_VB);
    glDeleteVertexArrays(1, &mbb_VA);
}

void Renderer::addBezierBox(glm::vec3 begin, glm::vec3 beginNormal, glm::vec3 end, glm::vec3 endNormal, glm::u8vec4 color)
{
    float segment = glm::length(end - begin) / 3.f;
    glm::vec3 pt1 = begin + beginNormal * segment;
    glm::vec3 pt2 = end - endNormal * segment;

    std::lock_guard lk(mem_mut);

    mbb_mem.emplace_back(bezierBox{
        {glm::vec4(begin, 1.f), glm::vec4(pt1, 1.f), glm::vec4(pt2, 1.f), glm::vec4(end, 1.f)},
        color, 
        0, // padding
        glm::vec2(0.1f, 0.1f)
    });
}

void Renderer::clearAll()
{
    std::lock_guard lk(mem_mut);

    mbb_mem.clear();
    mbb_indexSize = 0;

    m_shouldUpdate.store(true);
}

void Renderer::updateBuffers()
{
    if (!m_shouldUpdate.load()) return;
    
    std::lock_guard lk(mem_mut);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mbb_VB);
    glBufferData(GL_SHADER_STORAGE_BUFFER, mbb_mem.size() * sizeof(bezierBox), mbb_mem.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mbb_VB);
    mbb_indexSize = mbb_mem.size();

    m_shouldUpdate.store(false);
}

void Renderer::draw()
{
    updateBuffers();

    if (mbb_indexSize){
        glBindVertexArray(mbb_VA);
        glPatchParameteri(GL_PATCH_VERTICES, 1);
        glDrawArraysInstanced(GL_PATCHES, 0, 1, mbb_indexSize);
    }
}