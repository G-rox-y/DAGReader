#include "camera.hpp"

Camera::Camera() : m_position(glm::vec3(0.f, 0.f, -1.f)), m_rot(glm::identity<glm::quat>()), m_shouldRecalculate(true) 
{}

void Camera::pan(bool up, bool left, bool down, bool right){
    if (up && down) up = down = false;
    if (left && right) left = right = false;

    if (up) m_position += glm::vec3(0.f, m_position.z / 100.f, 0.f);
    if (left) m_position += glm::vec3(-m_position.z / 100.f, 0.f, 0.f);
    if (down) m_position += glm::vec3(0.f, -m_position.z / 100.f, 0.f);
    if (right) m_position += glm::vec3(m_position.z / 100.f, 0.f, 0.f);

    m_shouldRecalculate = true;
}

void Camera::zoom(bool in, bool out){
    if (in && out) return;

    // keep trying to update until you succeed (since its atomic we have to make sure it works)
    auto atomic_mul = [](std::atomic<float>& a, const float factor){
        float cur = a.load(std::memory_order_relaxed), next;
        do next = cur * factor;
        while (!a.compare_exchange_weak(cur, next, std::memory_order_release, std::memory_order_relaxed));
    };

    if (out && m_position.z < -0.1f) atomic_mul(m_scale, 1.f - m_zoom_sens);
    if (in) atomic_mul(m_scale, 1.f + m_zoom_sens);

    m_shouldRecalculate = true;
}

void Camera::rotate(bool up, bool left, bool down, bool right, bool cw, bool ccw){
    if (up && down) up = down = false;
    if (left && right) left = right = false;
    if (cw && ccw) cw = ccw = false;

    if (up) m_rot = glm::angleAxis(m_rotate_sens, glm::vec3(1.f, 0.f, 0.f)) * m_rot;
    if (down) m_rot = glm::angleAxis(-m_rotate_sens, glm::vec3(1.f, 0.f, 0.f)) * m_rot;
    if (left) m_rot = glm::angleAxis(m_rotate_sens, glm::vec3(0.f, 1.f, 0.f)) * m_rot;
    if (right) m_rot = glm::angleAxis(-m_rotate_sens, glm::vec3(0.f, 1.f, 0.f)) * m_rot;
    if (cw) m_rot = glm::angleAxis(m_rotate_sens, glm::vec3(0.f, 0.f, 1.f)) * m_rot;
    if (ccw) m_rot = glm::angleAxis(-m_rotate_sens, glm::vec3(0.f, 0.f, 1.f)) * m_rot;

    m_shouldRecalculate = true;
}

void Camera::setZoom(const float scale){
    m_scale.store(scale);
    
    m_shouldRecalculate = true;
}

void Camera::resetView(){ 
    m_scale = 1.f; 
    m_rot = glm::identity<glm::quat>(); 
    m_position = glm::vec3(0.f, 0.f, -1.f);

    m_shouldRecalculate = true;
}

void Camera::update(int window_w, int window_h){
    // check if any recalculations are supposed to happen
    if (!m_shouldRecalculate) return;
    m_shouldRecalculate = false;
    
    // recalculate
    m_mat = 
        glm::perspective(glm::radians(60.f), (float)window_w/window_h, 0.05f, 500.f)
        * glm::translate(glm::mat4(1.0f), m_position) * glm::mat4(m_rot) * glm::scale(glm::mat4(1), glm::vec3(m_scale));
    ;
}