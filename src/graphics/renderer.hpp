#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "pch.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include "GLProgram.hpp"

struct BezierBox{
    glm::dvec3 start, end;
    glm::dvec3 startOri, endOri;
    glm::dvec2 dims;
    glm::u8vec4 color;
};

// this class is thread safe since elements will be added through one thread and displayed through the other
class Renderer {
private:
    struct appearance {
        glm::u8vec4 color;
        uint32_t _padding0; // padding so that we can pass the data directly and have it comply with glsl std430
        glm::vec2 halfExt; // dimensions of the box cross-section
    };

    // --- data

    std::vector<std::array<glm::vec4, 4>> m_controlPoints;
    std::vector<appearance> m_appearances;
    GLuint m_indexSize = 0; // last saved vector size
    std::queue<std::pair<size_t, size_t>> m_needUpdating;
    bool m_resizeHappened = false;

    // groupIndices[x] contains all indices that are a part of group x
    // groupIndices[1] = vector[<1, 3>, <7,8>, <10, 10>] means 1,2,3,7,8,10 are elements of group 1
    std::unordered_map<int, std::vector<std::pair<size_t, size_t>>> m_groupIndices;
    std::unordered_set<int> m_active_groups;

    // --- opengl data

    GLuint m_VA; // id of the vertex array
    GLuint m_VB0; // id of the vertex buffer 0
    GLuint m_VB1; // id of the vertex buffer 1
    // ^ 2 buffers because we need 2 arrays, one for pts, and one for appearance

    // --- other
    mutable std::mt19937 m_rng{std::random_device{}()};

    void boxInsert(const BezierBox& b);

public:
    GLProgram program; // the shaders

    Renderer();
    ~Renderer();

    void changeGroupColors(const glm::u8vec4 newColor);
    void randomizeGroupColors();
    void changeGroupDims(const glm::vec2 dims);

    void activateGroup(const int id) { m_active_groups.insert(id); }
    void deactivateGroup(const int id) { m_active_groups.erase(id); }
    void deactivateAllGroups() { m_active_groups.clear(); }
    void setAsOnlyGroup(const int id) { deactivateAllGroups(); activateGroup(id); }

    void addBox(const BezierBox& box);
    void addBoxes(const std::vector<BezierBox>& boxes);

    void clearAll();

    // this function is going to fill the vertex buffer object with data and properly assign its vertex array
    void updateBuffers();

    void draw();
};