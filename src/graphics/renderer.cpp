#include "renderer.hpp"

void Renderer::boxInsert(const BezierBox& b){
    double segment = glm::length(b.end - b.start) / 3.f;
    glm::vec3 pt1 = b.start + b.startOri * segment;
    glm::vec3 pt2 = b.end + b.endOri * segment;

    m_controlPoints.emplace_back(std::array<glm::vec4, 4>{
        glm::vec4(b.start, 1.f), glm::vec4(pt1, 1.f), glm::vec4(pt2, 1.f), glm::vec4(b.end, 1.f)
    });
    m_appearances.emplace_back(appearance{
        b.color, 
        0, // padding
        b.dims
    });
}

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

void Renderer::changeGroupColors(const glm::u8vec4 newColor){
    for(const auto group:m_active_groups){
        for(const auto& p:m_groupIndices[group]){
            for(size_t i = p.first; i <= p.second; i++)
                m_appearances[i].color = newColor;
            m_needUpdating.push(p);
        }
    }
}

void Renderer::randomizeGroupColors(){
    static std::uniform_int_distribution<int> dist(0, 255);
    for(const auto group:m_active_groups){
        for(const auto& p:m_groupIndices[group]){
            for(size_t i = p.first; i <= p.second; i++)
                m_appearances[i].color = glm::u8vec4(dist(m_rng), dist(m_rng), dist(m_rng), 255);
            m_needUpdating.push(p);
        }
    }
}

void Renderer::changeGroupDims(const glm::vec2 dims){
    for(const auto group:m_active_groups){
        for(const auto& p:m_groupIndices[group]){
            for(size_t i = p.first; i <= p.second; i++)
                m_appearances[i].halfExt = dims;
            m_needUpdating.push(p);
        }
    }
}

void Renderer::addBox(const BezierBox& box){
    boxInsert(box);
    size_t newS = m_controlPoints.size() -1;

    for(const auto& group:m_active_groups)
        m_groupIndices[group].emplace_back(std::make_pair(newS, newS));
    
    m_resizeHappened = true;
}

void Renderer::addBoxes(const std::vector<BezierBox>& boxes){
    if (boxes.empty()) return;

    size_t oldS = m_controlPoints.size();
    for(const auto& b:boxes) boxInsert(b);
    size_t newS = m_controlPoints.size() -1;
    
    for(const auto group:m_active_groups)
        m_groupIndices[group].emplace_back(std::make_pair(oldS, newS));
    
    m_resizeHappened = true;
}

void Renderer::clearAll()
{
    m_controlPoints.clear();
    m_appearances.clear();
    m_indexSize = 0;

    deactivateAllGroups();
    m_groupIndices.clear();
    
    while(!m_needUpdating.empty()) m_needUpdating.pop();
    
    m_resizeHappened = true;
}

void Renderer::updateBuffers()
{
    static size_t arrsize = sizeof(std::array<glm::vec4, 4>), appsize = sizeof(appearance);

    if (m_appearances.size() != m_controlPoints.size())
        spdlog::error("Renderer error: Mismatch in bezier box array sizes");

    size_t old_indexSize = m_indexSize;
    m_indexSize = m_controlPoints.size();

    if (m_indexSize == 0){
        m_resizeHappened = false;
        return;
    }

    if (m_resizeHappened || m_indexSize != old_indexSize){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_VB0);
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_indexSize * arrsize, m_controlPoints.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_VB0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_VB1);
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_indexSize * appsize, m_appearances.data(), GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_VB1);

        while(!m_needUpdating.empty()) m_needUpdating.pop();
        m_resizeHappened = false;
    }
    else{
        while(!m_needUpdating.empty()){
            auto[start, end] = m_needUpdating.front(); m_needUpdating.pop();
            if (end >= m_indexSize) continue;
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_VB0);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, start * arrsize, (end-start+1)*arrsize, m_controlPoints.data() + start);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_VB1);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, start * appsize, (end-start+1)*appsize, m_appearances.data() + start);
        }
    }
}

void Renderer::draw()
{
    if (!m_needUpdating.empty() || m_resizeHappened)
        updateBuffers();

    if (m_indexSize){
        glBindVertexArray(m_VA);
        glPatchParameteri(GL_PATCH_VERTICES, 1);
        program.setUniform1i("SSBOffset", 0);
        glDrawArraysInstanced(GL_PATCHES, 0, 1, m_indexSize);
    }
}