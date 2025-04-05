#pragma once

#include "pch.hpp"

class Camera {
private:
    // the camera position, panning is controlled with x and y, zooming with z
    glm::vec3 m_position;
    
    // the (camera) view matrix
    glm::mat4 m_mat;

    // when a change in position or rotation occurs update() should recalculate the matrix
    // this bool is used to make sure that no unnecesarry updates happen
    bool m_shouldRecalculate;
public:
    Camera();
    Camera(const glm::vec3& pos);
    ~Camera() = default;

    void pan(bool up, bool left, bool down, bool right);
    void zoom(bool in, bool out);

    // this should get called before drawing the frame
    // checks if camera matrix recalculation is needed, if yes then it recalculates
    void update(int window_w, int window_h);

    void setPosition(const glm::vec3& pos) { m_position = pos; }

    bool& getRecalc() { return m_shouldRecalculate; }
    const glm::mat4& getMat() const { return m_mat; }
};
