#pragma once

#include "pch.hpp"

class Camera {
private:
    glm::vec3 m_position; // the camera position
    glm::quat m_rot; // the model rotation
    float m_scale = 1.f; // this parameter will be used for zooming (model scale)
    // m_scale is atomic because 

    // the (camera) view matrix
    glm::mat4 m_mat;

    // when a change in position or rotation occurs update() should recalculate the matrix
    // this bool is used to make sure that no unnecesarry updates happen
    bool m_shouldRecalculate;

    // control sensitivities
    float m_scale_sens = 0.02f;
    float m_rotate_sens = glm::radians(0.75f); // in degrees per update (one update per frame)
    float m_pan_sens = 0.02f;
    // TODO: this will rotate slower if the framerate drops, the update functions should get a dt parameter
    // and sensitivities should be calculated in degrees/second not per update at some point

    // camera FOV
    float m_FOV = glm::radians(60.f);

public:
    Camera();
    ~Camera() = default;

    void move(bool up, bool left, bool down, bool right, bool in, bool out, bool fast);
    void scale(bool in, bool out);

    void rotate(bool up, bool left, bool down, bool right, bool cw, bool ccw, bool fast);

    // this should get called before drawing the frame
    // checks if camera matrix recalculation is needed, if yes then it recalculates
    void update(int window_w, int window_h);

    void resetView();

    bool& getRecalc() { return m_shouldRecalculate; }
    const glm::mat4& getMat() const { return m_mat; }
    const glm::vec3 getPos() const { return -m_position / m_scale; } // returning the negative because it makes more sense
    const float getFOV() const { return m_FOV; }
    const float getScale() const { return m_scale; }
};
