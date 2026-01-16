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
    glGenBuffers(1, &m_VB2);
    glGenBuffers(1, &m_AC0);
}

Renderer::~Renderer()
{
    glDeleteBuffers(1, &m_AC0);
    glDeleteBuffers(1, &m_VB2);
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
            for(size_t i = p.first; i <= p.second; i++){
                int alpha = m_appearances[i].color.w; // preserve transparancy value
                m_appearances[i].color = glm::u8vec4(dist(m_rng), dist(m_rng), dist(m_rng), alpha);
            }
            m_needUpdating.push(p);
        }
    }
}

void Renderer::changeGroupDims(const glm::vec2& dims){
    for(const auto group:m_active_groups){
        for(const auto& p:m_groupIndices[group]){
            for(size_t i = p.first; i <= p.second; i++)
                m_appearances[i].halfExt = dims;
            m_needUpdating.push(p);
        }
    }
}

void Renderer::activateGroup(const int id){
    if (m_active_groups.find(id) != m_active_groups.end()) return;
    m_active_groups.insert(id);
    for(const auto group: m_groupSubgroups[id])
        activateGroup(group);
}

void Renderer::deactivateGroup(const int id){
    if (m_active_groups.find(id) == m_active_groups.end()) return;
    m_active_groups.erase(id);
    for(const auto group: m_groupSubgroups[id])
        deactivateGroup(group);
}

void Renderer::deactivateAllGroups(){
    m_active_groups.clear();
}

void Renderer::setAsOnlyGroup(const int id){
    deactivateAllGroups();
    activateGroup(id);
}

void Renderer::makeXSubgroupOfY(const int X, const int Y){
    m_groupSubgroups[Y].insert(X);
}

void Renderer::removeXAsSubgroupOfY(const int X, const int Y){
    m_groupSubgroups[Y].erase(X);
}

bool Renderer::isEntryInGroup(const std::pair<size_t, size_t>& entry, const int id){
    size_t i = m_groupIndices[id].size() -1;
    while(i+1 != 0 && m_groupIndices[id][i] != entry) i--;
    return i+1!=0;
}

void Renderer::addEntryToGroup(const std::pair<size_t, size_t>& entry, const int id){
    m_groupIndices[id].push_back(entry);
}

void Renderer::removeEntryFromGroup(const std::pair<size_t, size_t>& entry, const int id){
    size_t i = m_groupIndices[id].size() -1;
    while(i+1 != 0 && m_groupIndices[id][i] != entry) i--;
    if (i+1==0) return;
    m_groupIndices[id].erase(m_groupIndices[id].begin() + i);
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

void Renderer::modifyPointColor(const size_t ID, const glm::u8vec4 color) {
    if (m_appearances.size() < ID){
        spdlog::warn("Trying to modify appearance out of range!");
        return;
    }
    m_appearances[ID].color = color;
}

void Renderer::clearAll()
{
    m_controlPoints.clear();
    m_appearances.clear();
    m_indexSize = 0;

    deactivateAllGroups();
    m_groupIndices.clear();
    m_groupSubgroups.clear();
    
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
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_VB2);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(selections), nullptr, GL_DYNAMIC_READ);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_VB2);
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_AC0);
        glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_AC0);

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
        // reset ac0
        static GLuint zero = 0;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_AC0);
        glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);
        
        glBindVertexArray(m_VA);
        glPatchParameteri(GL_PATCH_VERTICES, 1);
        glDrawArraysInstanced(GL_PATCHES, 0, 1, m_indexSize);

        // read shader output (closest point to mouse vector)
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
    
        static GLuint count;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_AC0);
        glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &count);
        count = std::min<int>(count, 16);
    
        if (count == 0) return;
    
        static selections results;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_VB2);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(selections), &results);
    
        int bestID = -1; float bestDist = 1e5;
        for(GLuint i = 0; i < count; i++){
            if (results.dists[i] < bestDist){
                bestID = results.ids[i];
                bestDist = results.dists[i];
            }
        }
        m_selectionID = bestID;
    }

}