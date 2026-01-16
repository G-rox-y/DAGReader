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
    // grouplinks[x] contains ids of all subgroups
    std::unordered_map<int, std::unordered_set<int>> m_groupSubgroups;
    std::unordered_set<int> m_active_groups;

    // --- opengl data

    GLuint m_VA; // id of the vertex array
    GLuint m_VB0; // id of the vertex buffer 0
    GLuint m_VB1; // id of the vertex buffer 1
    // ^ 2 buffers because we need 2 arrays, one for pts, and one for appearance
    GLuint m_VB2; // this one is for the selection retrieval data
    GLuint m_AC0; // the atomic counter sor the VB2

    // --- shader info return

    struct selections {
        int ids[16];
        float dists[16];
    };

    int m_selectionID = -1;

    // --- other
    mutable std::mt19937 m_rng{std::random_device{}()};

    void boxInsert(const BezierBox& b);

public:
    GLProgram program; // the shaders

    Renderer();
    ~Renderer();

    void changeGroupColors(const glm::u8vec4 newColor);
    void randomizeGroupColors();
    void changeGroupDims(const glm::vec2& dims);

    void activateGroup(const int id);
    void deactivateGroup(const int id);
    void deactivateAllGroups();
    void setAsOnlyGroup(const int id);
    void makeXSubgroupOfY(const int X, const int Y);
    void removeXAsSubgroupOfY(const int X, const int Y);

    bool isEntryInGroup(const std::pair<size_t, size_t>& entry, const int id);
    void addEntryToGroup(const std::pair<size_t, size_t>& entry, const int id);
    void removeEntryFromGroup(const std::pair<size_t, size_t>& entry, const int id);

    void addBox(const BezierBox& box);
    void addBoxes(const std::vector<BezierBox>& boxes);

    void modifyPointColor(const size_t ID, const glm::u8vec4 color);

    void clearAll();

    // this function is going to fill the vertex buffer object with data and properly assign its vertex array
    void updateBuffers();

    void draw();

    // returns -1 if nothing is near the mouse
    int getNearestToMouse() const { return m_selectionID; }
};