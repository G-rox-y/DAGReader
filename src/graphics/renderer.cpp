#include "renderer.hpp"


Renderer::Renderer() : program()
{
    glGenVertexArrays(1, &m_VA);
    glGenBuffers(1, &m_VB0);
    glGenBuffers(1, &m_VB1);
}

Renderer::~Renderer()
{
    glDeleteBuffers(1, &m_VB1);
    glDeleteBuffers(1, &m_VB0);
    glDeleteVertexArrays(1, &m_VA);
}

void Renderer::addSegment(
    const glm::vec3& begin, const glm::vec3& beginNormal, 
    const glm::vec3& end, const glm::vec3& endNormal, 
    const glm::vec2& dimensions, glm::u8vec4 color
){
    float segment = glm::length(end - begin) / 3.f;
    glm::vec3 pt1 = begin + beginNormal * segment;
    glm::vec3 pt2 = end - endNormal * segment;

    m_seg_controlPoints.emplace_back(std::array<glm::vec4, 4>{
        glm::vec4(begin, 1.f), glm::vec4(pt1, 1.f), glm::vec4(pt2, 1.f), glm::vec4(end, 1.f)
    });
    m_seg_appearances.emplace_back(appearance{
        color, 
        0, // padding
        dimensions
    });
}

void Renderer::addLink(
    const glm::vec3& begin, const glm::vec3& beginNormal, 
    const glm::vec3& end, const glm::vec3& endNormal, 
    const glm::vec2& dimensions, glm::u8vec4 color
){
    float segment = glm::length(end - begin) / 3.f;
    glm::vec3 pt1 = begin + beginNormal * segment;
    glm::vec3 pt2 = end - endNormal * segment;

    m_link_controlPoints.emplace_back(std::array<glm::vec4, 4>{
        glm::vec4(begin, 1.f), glm::vec4(pt1, 1.f), glm::vec4(pt2, 1.f), glm::vec4(end, 1.f)
    });
    m_link_appearances.emplace_back(appearance{
        color, 
        0, // padding
        dimensions
    });
}

void Renderer::changeSegmentColors(glm::u8vec4 newColor){
    for(auto& e:m_seg_appearances) e.color = newColor;
    m_seg_appearanceUpdated = true;
    m_seg_colorsRandomized = false;
}

void Renderer::changeLinkColors(glm::u8vec4 newColor){
    for(auto& e:m_link_appearances) e.color = newColor;
    m_link_appearanceUpdated = true;
    m_link_colorsRandomized = false;
}

void Renderer::randomizeSegmentColors(){
    if (m_seg_colorsRandomized) return;
    static std::uniform_int_distribution<int> dist(0, 255);
    for(auto& e:m_seg_appearances)
        e.color = glm::u8vec4(dist(m_rng), dist(m_rng), dist(m_rng), 255);
    m_seg_colorsRandomized = true;
    m_seg_appearanceUpdated = true;
}

void Renderer::randomizeLinkColors(){
    if (m_link_colorsRandomized) return;
    static std::uniform_int_distribution<int> dist(0, 255);
    for(auto& e:m_link_appearances)
        e.color = glm::u8vec4(dist(m_rng), dist(m_rng), dist(m_rng), 255);
    m_link_colorsRandomized = true;
    m_link_appearanceUpdated = true;
}

void Renderer::clearAll()
{
    m_seg_controlPoints.clear();
    m_seg_appearances.clear();
    m_seg_indexSize = 0;

    m_seg_appearanceUpdated = m_seg_pointsUpdated = true;

    m_link_controlPoints.clear();
    m_link_appearances.clear();
    m_link_indexSize = 0;

    m_link_appearanceUpdated = m_link_pointsUpdated = true;
}

void Renderer::updateBuffers()
{    
    if (m_seg_controlPoints.size() != m_seg_appearances.size() || m_link_controlPoints.size() != m_link_appearances.size())
        spdlog::error("Renderer error: Mismatch in bezier box array sizes");
    
    size_t m_old_seg_indexSize = m_seg_indexSize;
    size_t m_old_link_indexSize = m_link_indexSize;
    m_seg_indexSize = m_seg_controlPoints.size();
    m_link_indexSize = m_link_controlPoints.size();

    static size_t arrsize = sizeof(std::array<glm::vec4, 4>), appsize = sizeof(appearance);

    // save control points in the buffer 0 and appearance data in the buffer 1

    if (m_seg_pointsUpdated || m_link_pointsUpdated){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_VB0);
        if (m_old_seg_indexSize == m_seg_indexSize){
            if (m_link_pointsUpdated)
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_link_indexSize * arrsize, m_link_controlPoints.data());
            if (m_seg_pointsUpdated)
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, m_link_indexSize * arrsize, m_seg_indexSize * arrsize, m_seg_controlPoints.data());
        }
        else{
            std::vector<std::array<glm::vec4, 4>> pts(m_seg_controlPoints);
            pts.insert(pts.begin(), m_link_controlPoints.begin(), m_link_controlPoints.end());
            glBufferData(GL_SHADER_STORAGE_BUFFER, pts.size() * arrsize, pts.data(), GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_VB0);
        }
        m_link_pointsUpdated = m_seg_pointsUpdated = false;
        
    }
    if (m_seg_appearanceUpdated || m_link_appearanceUpdated){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_VB1);
        if (m_old_link_indexSize == m_link_indexSize){
            if (m_link_appearanceUpdated)
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_link_indexSize * appsize, m_link_appearances.data());
            if (m_seg_appearanceUpdated)
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, m_link_indexSize * appsize, m_seg_indexSize * appsize, m_seg_appearances.data());
        }
        else{
            std::vector<appearance> apps(m_seg_appearances);
            apps.insert(apps.begin(), m_link_appearances.begin(), m_link_appearances.end());
            glBufferData(GL_SHADER_STORAGE_BUFFER, apps.size() * appsize, apps.data(), GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_VB1);
        }
        m_link_appearanceUpdated = m_seg_appearanceUpdated = false;
    }
}

void Renderer::draw()
{
    if (m_seg_appearanceUpdated || m_seg_pointsUpdated || m_link_appearanceUpdated || m_link_pointsUpdated)
        updateBuffers();

    if (m_seg_indexSize || m_link_indexSize){
        glBindVertexArray(m_VA);
        glPatchParameteri(GL_PATCH_VERTICES, 1);
        program.setUniform1i("SSBOffset", 0);
        glDrawArraysInstanced(GL_PATCHES, 0, 1, m_link_indexSize);
        program.setUniform1i("SSBOffset", (int)m_link_indexSize);
        glDrawArraysInstanced(GL_PATCHES, 0, 1, m_seg_indexSize);
    }
}