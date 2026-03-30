#include "camera.hpp"

void Camera::orbitUpdate(){
    if (!m_orbitingActive) return;

    glm::vec3 C2P = m_orbitPoint - m_position;
    glm::vec3 NP = glm::cross(C2P, m_orbitPointNormal);
    C2P = normalize(glm::normalize(C2P) + glm::normalize(NP) * 0.01f);
    
    m_position = m_orbitPoint - C2P * m_orbitDistance;

    glm::vec3 right = glm::normalize(glm::cross(m_orbitPointNormal, C2P));
    glm::vec3 newUp = glm::cross(C2P, right);
    glm::mat3 rotMat(right, newUp, C2P);
    m_rot = glm::quat_cast(rotMat);

    m_shouldRecalculate = true;
}

Camera::Camera() : m_position(m_default_position), m_rot(glm::identity<glm::quat>()), m_scale(m_default_scale) 
{}

void Camera::move(bool up, bool left, bool down, bool right, bool in, bool out, bool fast){
    if (m_orbitingActive){
        if (up || down || left || right) m_orbitingActive = false;
        else{
            if (in && out) return;
            if (in) m_orbitDistance *= 0.98f;
            else if (out) m_orbitDistance *= 1.03f;
            return;
        }
    }

    if (up && down) up = down = false;
    if (left && right) left = right = false;
    if (in && out) in = out = false;

    float increment = (fast) ? m_pan_sens * 10.f : m_pan_sens;

    if (up) m_position += glm::vec3(0.f, -increment, 0.f) * m_rot;
    if (left) m_position += glm::vec3(increment, 0.f, 0.f) * m_rot;
    if (down) m_position += glm::vec3(0.f, increment, 0.f) * m_rot;
    if (right) m_position += glm::vec3(-increment, 0.f, 0.f) * m_rot;
    if (in) m_position += glm::vec3(0.f, 0.f, increment) * m_rot;
    if (out) m_position += glm::vec3(0.f, 0.f, -increment) * m_rot;

    m_shouldRecalculate = true;
}

void Camera::scale(bool in, bool out){
    if (m_orbitingActive) return;

    if (in && out) return;

    float prevscale = m_scale;
    if (out) m_scale *= 1.f - m_scale_sens;
    if (in) m_scale *= 1.f + m_scale_sens;
    m_position *= m_scale / prevscale;

    m_shouldRecalculate = true;
}

void Camera::rotate(bool up, bool left, bool down, bool right, bool cw, bool ccw, bool fast){
    if (m_orbitingActive){
        if (up || down || left || right) m_orbitingActive = false;
        else{
            glm::vec3 C2P = m_orbitPoint - m_position;
            glm::vec3 NP = glm::cross(C2P, m_orbitPointNormal);
            if (cw && ccw) return;
            if (cw) m_orbitPointNormal = normalize(m_orbitPointNormal + glm::normalize(NP) * 0.01f);
            else if (ccw) m_orbitPointNormal = normalize(m_orbitPointNormal - glm::normalize(NP) * 0.01f);
            return;
        }
    }

    if (up && down) up = down = false;
    if (left && right) left = right = false;
    if (cw && ccw) cw = ccw = false;

    float increment = (fast) ? m_rotate_sens * 2.f : m_rotate_sens;

    if (up) m_rot = glm::angleAxis(-increment, glm::vec3(1.f, 0.f, 0.f)) * m_rot;
    if (down) m_rot = glm::angleAxis(increment, glm::vec3(1.f, 0.f, 0.f)) * m_rot;
    if (left) m_rot = glm::angleAxis(-increment, glm::vec3(0.f, 1.f, 0.f)) * m_rot;
    if (right) m_rot = glm::angleAxis(increment, glm::vec3(0.f, 1.f, 0.f)) * m_rot;
    if (cw) m_rot = glm::angleAxis(increment, glm::vec3(0.f, 0.f, 1.f)) * m_rot;
    if (ccw) m_rot = glm::angleAxis(-increment, glm::vec3(0.f, 0.f, 1.f)) * m_rot;

    m_shouldRecalculate = true;
}

void Camera::engageOrbit(const glm::vec3& center, const float distance){
    m_orbitPoint = -center * m_scale;
    m_orbitPointNormal = glm::normalize(glm::vec3(0.f, -1.f, 0.f));
    m_orbitDistance = distance * m_scale;
    
    glm::vec3 C2P = m_orbitPoint - m_position;
    glm::vec3 PNProj = glm::dot(C2P, m_orbitPointNormal) * m_orbitPointNormal;
    C2P = glm::normalize(C2P - PNProj);
    m_position = m_orbitPoint - C2P * m_orbitDistance;
    
    glm::vec3 right = -glm::normalize(glm::cross(m_orbitPointNormal, C2P));
    glm::vec3 newUp = glm::cross(C2P, right);
    glm::mat3 rotMat(right, newUp, C2P);
    m_rot = glm::quat_cast(rotMat);

    m_orbitingActive = true;
}

void Camera::disengageOrbit() { m_orbitingActive = false; }

void Camera::resetView(){ 
    m_scale = m_default_scale;
    m_rot = glm::identity<glm::quat>(); 
    m_position = m_default_position * m_default_scale;

    m_shouldRecalculate = true;
    m_orbitingActive = false;
}

void Camera::update(int window_w, int window_h){
    if (m_orbitingActive) orbitUpdate();

    // check if any recalculations are supposed to happen
    if (!m_shouldRecalculate) return;
    m_shouldRecalculate = false;

    // recalculate
    m_mat = glm::perspective(m_FOV, (float)window_w/window_h, m_nearCP, m_farCP)
        * glm::mat4(m_rot) * glm::translate(glm::mat4(1.0f), m_position) * glm::scale(glm::mat4(1), glm::vec3(m_scale))
    ;
}