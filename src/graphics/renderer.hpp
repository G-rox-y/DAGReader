#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "pch.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include "GLProgram.hpp"

// this class is thread safe since elements will be added through one thread and displayed through the other
class Renderer {
private:
    struct appearance {
        glm::u8vec4 color;
        uint32_t _padding0; // padding so that we can pass the data directly and have it comply with glsl std430
        glm::vec2 halfExt; // dimensions of the box cross-section
    };

    // --- data for segments

    std::vector<std::array<glm::vec4, 4>> m_seg_controlPoints;
    std::vector<appearance> m_seg_appearances;
    GLuint m_seg_indexSize = 0; // the saved vector size between updates

    bool m_seg_pointsUpdated = false;
    bool m_seg_appearanceUpdated = false;

    bool m_seg_colorsRandomized = false;

    // --- data for links

    std::vector<std::array<glm::vec4, 4>> m_link_controlPoints;
    std::vector<appearance> m_link_appearances;
    GLuint m_link_indexSize = 0; // the saved vector size between updates

    bool m_link_pointsUpdated = false;
    bool m_link_appearanceUpdated = false;

    bool m_link_colorsRandomized = false;

    // --- other / common data

    GLuint m_VA; // id of the vertex array
    GLuint m_VB0; // id of the vertex buffer 0
    GLuint m_VB1; // id of the vertex buffer 1
    // ^ 2 buffers because we need 2 arrays, one for pts, and one for appearance

    mutable std::mt19937 m_rng{std::random_device{}()};

public:
    GLProgram program; // the shaders

    Renderer();
    ~Renderer();

    void addSegment(
        const glm::vec3& begin, const glm::vec3& beginNormal, 
        const glm::vec3& end, const glm::vec3& endNormal, 
        const glm::vec2& dimensions, glm::u8vec4 color
    );
    void addLink(
        const glm::vec3& begin, const glm::vec3& beginNormal, 
        const glm::vec3& end, const glm::vec3& endNormal, 
        const glm::vec2& dimensions, glm::u8vec4 color
    );

    void changeSegmentColors(glm::u8vec4 newColor);
    void changeLinkColors(glm::u8vec4 newColor);

    void randomizeSegmentColors();
    void randomizeLinkColors();

    void clearAll();

    // this function is going to fill the vertex buffer object with data and properly assign its vertex array
    void updateBuffers();

    void setSegShouldUpdate() { m_seg_appearanceUpdated = m_seg_pointsUpdated = true; }
    void setLinkShouldUpdate() { m_link_appearanceUpdated = m_link_pointsUpdated = true; }
    void setShouldUpdate() { setSegShouldUpdate(); setLinkShouldUpdate(); }

    void draw();
};