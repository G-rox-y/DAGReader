#include "renderer.hpp"

void Renderer::boxInsert(const BezierBox& b){
    double segment = glm::length(b.end - b.start) / 3.f;
    glm::vec3 pt1 = b.start + b.startOri * segment;
    glm::vec3 pt2 = b.end + b.endOri * segment;

    m_controlPoints.emplace_back(std::array<glm::vec4, 4>{
        glm::vec4(b.start, 1.f), glm::vec4(pt1, 1.f), glm::vec4(pt2, 1.f), glm::vec4(b.end, 1.f)
    });
    m_appearances.emplace_back(appearance{b.color, b.dims});
}

void Renderer::changeGroupColors(const glm::u8vec4 newColor){
    std::lock_guard lk(m_ownership);
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);
    for(const auto group:m_active_groups){
        for(const auto& p:m_groupIndices[group]){
            for(size_t i = p.first; i <= p.second; i++)
                m_appearances[i].color = newColor;
            m_needUpdating.push(p);
        }
    }
}

void Renderer::randomizeGroupColors(){
    std::lock_guard lk(m_ownership);
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);
    static std::uniform_int_distribution<int> dist(0, 255);
    for(const auto group:m_active_groups){
        for(const auto& p:m_groupIndices[group]){
            for(size_t i = p.first; i <= p.second; i++){
                int alpha = m_appearances[i].color.w; // preserve transparency value
                m_appearances[i].color = glm::u8vec4(dist(m_rng), dist(m_rng), dist(m_rng), alpha);
            }
            m_needUpdating.push(p);
        }
    }
}

void Renderer::changeGroupDims(const glm::vec2& dims){
    std::lock_guard lk(m_ownership);
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);
    for(const auto group:m_active_groups){
        for(const auto& p:m_groupIndices[group]){
            for(size_t i = p.first; i <= p.second; i++)
                m_appearances[i].halfExt = dims;
            m_needUpdating.push(p);
        }
    }
}

void Renderer::activateGroup(const int id) {
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);
    m_active_groups.insert(id);
}

void Renderer::deactivateGroup(const int id) {
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);
    m_active_groups.erase(id);
}

bool Renderer::isEntryInGroup(const IndexRange& entry, const int id){
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);
    // this will have to be a linear search (list)
    for(const auto& p:m_groupIndices[id])
        if (p.first <= entry.first && p.second >= entry.second) return true;
    
    return false;    
}

void Renderer::addEntryToGroup(const IndexRange& entry, const int id){
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);
    auto& ranges = m_groupIndices[id];
    size_t mergedStart = entry.first;
    size_t mergedEnd = entry.second;
    
    // find and merge all overlapping/adjacent ranges
    for (auto it = ranges.begin(); it != ranges.end(); ) {
        auto& p = *it;
        
        if (entry.second < p.first) {
            ranges.insert(it, {mergedStart, mergedEnd});
            return;
        }
        if (entry.first > p.second) {
            it++;
            continue;
        }
        
        mergedStart = std::min(mergedStart, p.first);
        mergedEnd = std::max(mergedEnd, p.second);
        it = ranges.erase(it);
    }
    
    // insert merged range at the end if we reached this pt
    ranges.push_back({mergedStart, mergedEnd});
}

void Renderer::removeEntryFromGroup(const IndexRange& entry, const int id){
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);
    auto& ranges = m_groupIndices[id];

    for(auto it = ranges.begin(); it != ranges.end(); ){
        auto& p = *it;
        
        if (entry.second < p.first) return;
        
        if (entry.first > p.second) it++;
        else{
            if (entry.first <= p.first){
                if (entry.second >= p.second) it = ranges.erase(it);
                else{
                    p.first = entry.second+1;
                    return;
                }
            }
            else{
                auto remember = p.second;
                p.second = entry.first-1;
                if (entry.second < remember){
                    ranges.insert(it, {entry.second+1, remember});
                    return;
                }
                it++;
            }
        }
    }
}

void Renderer::addGroupXToY(const int X, const int Y){
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);
    for(const auto& entry:m_groupIndices[X])
        addEntryToGroup(entry, Y);
}

void Renderer::removeGroupXFromY(const int X, const int Y){
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);
    for(const auto& entry:m_groupIndices[X])
        removeEntryFromGroup(entry, Y);
}

void Renderer::addBoxes(const std::vector<BezierBox>& boxes){
    if (boxes.empty()) return;

    std::lock_guard lk(m_ownership);
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);

    size_t oldS = m_controlPoints.size();
    for(const auto& b:boxes) boxInsert(b);
    size_t newS = m_controlPoints.size() -1;
    
    for(const auto group:m_active_groups)
        m_groupIndices[group].emplace_back(std::make_pair(oldS, newS));
    
    m_resizeHappened = true;
}

void Renderer::clearAll()
{
    std::lock_guard lk(m_ownership);
    std::lock_guard<std::recursive_mutex> lk2(m_group_lock);

    m_controlPoints.clear();
    m_appearances.clear();
    m_indexSize = 0;

    m_selectionID = -1;

    m_active_groups.clear(); 
    m_groupIndices.clear();
    
    while(!m_needUpdating.empty()) m_needUpdating.pop();
    
    m_resizeHappened = true;
}

void Renderer::updateBuffers()
{
    static size_t arrsize = sizeof(std::array<glm::vec4, 4>), appsize = sizeof(appearance);

    if (m_appearances.size() != m_controlPoints.size())
        spdlog::error("Renderer error: Mismatch in bezier box array sizes; Appearances: {} | ControlPoints: {}",
            m_appearances.size(), m_controlPoints.size());

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
    std::unique_lock lk(m_ownership, std::try_to_lock); // try to lock the ownership mutex
    if (!lk.owns_lock()) return; // if you fail, its ok to skip the frame(s)

    if (!m_needUpdating.empty() || m_resizeHappened)
        updateBuffers();

    
    if (m_indexSize){
        // reset ac0
        GLuint zero = 0;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_AC0);
        glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);
        
        glBindVertexArray(m_VA);
        glPatchParameteri(GL_PATCH_VERTICES, 1);
        glDrawArraysInstanced(GL_PATCHES, 0, 1, m_indexSize);

        // read shader output (closest point to mouse vector)
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
    
        GLuint count;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_AC0);
        glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &count);
        count = std::min<int>(count, MAX_SELECTIONS);
        
        selections results;
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

bool Renderer::addMouseSelectionToGroup(const int id){
    std::lock_guard lk(m_ownership);
    if (m_selectionID != -1){
        auto entry = std::make_pair(m_selectionID, m_selectionID);
        if (isEntryInGroup(entry, id))
            removeEntryFromGroup(entry, id);
        else addEntryToGroup(entry, id);
        return true;
    }
    else return false;
}