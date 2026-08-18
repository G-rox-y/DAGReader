#pragma once

#include "pch.hpp"

class Camera {
private:
    // the camera position
    glm::vec3 m_default_position = glm::vec3(0.f);
    glm::vec3 m_position;

    glm::quat m_rot; // the model rotation

    // this parameter will be used for zooming (model scale)
    float m_default_scale = 1.f;
    float m_scale;

    // the (camera) view matrix
    glm::mat4 m_mat;

    // when a change in position or rotation occurs update() should recalculate the matrix
    // this bool is used to make sure that no unnecesarry updates happen
    bool m_shouldRecalculate = true;

    // control sensitivities
    float m_scale_sens = 0.02f;
    float m_rotate_sens = glm::radians(1.f); // in degrees per update (one update per frame)
    float m_pan_sens = 0.02f;
    // TODO: this will rotate slower if the framerate drops, the update functions should get a dt parameter
    // and sensitivities should be calculated in degrees/second not per update at some point

    // camera FOV
    float m_FOV = glm::radians(60.f);

    // clipping planes
    float m_nearCP = 0.005f;
    float m_farCP = 500.f;

    // animation related variables
    bool m_orbitingActive = false;
    float m_orbitDistance;
    glm::vec3 m_orbitPoint, m_orbitPointNormal;
    void orbitUpdate();

public:
    Camera();
    ~Camera() = default;

    void move(bool up, bool left, bool down, bool right, bool in, bool out, bool fast);
    void scale(bool in, bool out);

    void rotate(bool up, bool left, bool down, bool right, bool cw, bool ccw, bool fast);

    void engageOrbit(const glm::vec3& center, const float distance);
    void disengageOrbit();

    // this should get called before drawing the frame
    // checks if camera matrix recalculation is needed, if yes then it recalculates
    void update(int window_w, int window_h);

    void resetView();

    void setDefScale(const float newscale) { m_default_scale = newscale; }
    void setDefPos(const glm::vec3& newpos) { m_default_position = -newpos; }

    // notify camera it should recalculate
    void shouldRecalc() { m_shouldRecalculate = true; }
    
    const glm::mat4& getMat() const { return m_mat; }
    glm::vec3 getPos() const { return -m_position / m_scale; } // returning the negative because it makes more sense
    float getFOV() const { return m_FOV; }
    float getScale() const { return m_scale; }
    float getFarCP() const { return m_farCP; }
    float getMovementSpeed() const { return m_pan_sens / m_scale; }
};
