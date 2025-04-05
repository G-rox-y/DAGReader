#include "camera.hpp"

Camera::Camera() : m_position(glm::vec3(0.f, 0.f, -1.f)), m_shouldRecalculate(true) 
{}

Camera::Camera(const glm::vec3& pos) : m_position(pos), m_shouldRecalculate(true)
{}

void Camera::pan(bool up, bool left, bool down, bool right){
    if (up && down) up = down = false;
    if (left && right) left = right = false;

    if (up) m_position += glm::vec3(0.f, -m_position.z / 100.f, 0.f);
    if (left) m_position += glm::vec3(m_position.z / 100.f, 0.f, 0.f);
    if (down) m_position += glm::vec3(0.f, m_position.z / 100.f, 0.f);
    if (right) m_position += glm::vec3(-m_position.z / 100.f, 0.f, 0.f);

    m_shouldRecalculate = true;
}

void Camera::zoom(bool in, bool out){
    if (in && out) return;

    if (in && m_position.z < -0.1f) m_position += glm::vec3(0.f, 0.f, m_position.z / 50.f);
    if (out) m_position += glm::vec3(0.f, 0.f, -m_position.z / 50.f);

    m_shouldRecalculate = true;
}

void Camera::update(int window_w, int window_h){
    // check if any recalculations are supposed to happen
    if (!m_shouldRecalculate) return;
    m_shouldRecalculate = false;
    
    // recalculate
    m_mat = glm::perspective(glm::radians(60.f), (float)window_w/window_h, 0.05f, 500.f) * glm::translate(glm::mat4(1.0f), m_position);
}