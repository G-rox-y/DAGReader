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
    size_t originalID;
};

// try to keep this a thread safe class
class Renderer {
public:
    // adding this just for better readability
    using IndexRange = std::pair<size_t, size_t>;
private:
    struct appearance {
        glm::u8vec4 color;
        uint32_t : 32; // padding so that we can pass the data directly and have it comply with glsl std430
        glm::vec2 halfExt; // dimensions of the box cross-section
    };

    // --- data

    std::vector<std::array<glm::vec4, 4>> m_controlPoints;
    std::vector<appearance> m_appearances;
    GLuint m_indexSize = 0; // last saved vector size
    std::queue<IndexRange> m_needUpdating;
    bool m_resizeHappened = false;

    // groupIndices[x] contains all indices that are a part of group x
    // groupIndices[1] = list[<1, 3>, <7,8>, <10, 10>] means 1,2,3,7,8,10 are elements of group 1
    // special care should be taken to keep this list sorted!
    std::unordered_map<int, std::list<IndexRange>> m_groupIndices;
    std::unordered_set<int> m_active_groups;

    std::unordered_map<int, int> m_rendererID2OldID;

    // --- opengl data

    // RAII wrapper for buffers and arrays
    struct GLObjectBase {
        GLuint id = 0;

        // no copy!!
        GLObjectBase(const GLObjectBase&) = delete;
        GLObjectBase& operator=(const GLObjectBase&) = delete;
        
        // when called without a method, return the ID
        operator GLuint() const { return id; }
    protected:
        GLObjectBase() = default;
        ~GLObjectBase() = default;
    };
    struct GLBuffer : GLObjectBase {
        GLBuffer() { glGenBuffers(1, &id); }
        ~GLBuffer() { if (id) glDeleteBuffers(1, &id); }
    };
    struct GLVertexArray : GLObjectBase {
        GLVertexArray() { glGenVertexArrays(1, &id); }
        ~GLVertexArray() { if (id) glDeleteVertexArrays(1, &id); }
    };

    // VB0 and VB1 are for control points and appearances
    // VB2 is for selection retrieval data and AC0 is its atomic counter
    GLBuffer m_VB0, m_VB1, m_VB2, m_AC0;
    GLVertexArray m_VA; // id of the vertex array

    // --- shader info return
    static constexpr size_t MAX_SELECTIONS = 16;

    struct selections {
        int ids[MAX_SELECTIONS];
        float dists[MAX_SELECTIONS];
    };

    int m_selectionID = -1;

    // --- other
    std::mt19937 m_rng{std::random_device{}()};

    void boxInsert(const BezierBox& b);

    // this function is going to fill the vertex buffer object with data and properly assign its vertex array
    void updateBuffers();

    GLProgram m_program; // the shaders
    
    mutable std::mutex m_ownership; // ownership mutex to keep the renderer thread safe
    mutable std::recursive_mutex m_group_lock; // group modification mutex
public:
    Renderer() = default;
    ~Renderer() = default;
    // but not meant to be copied!!
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void changeGroupColors(const glm::u8vec4 newColor);
    void randomizeGroupColors();
    void changeGroupDims(const glm::vec2& dims);

    void activateGroup(const int id);
    void deactivateGroup(const int id);

    bool isEntryInGroup(const IndexRange& entry, const int id);
    void addEntryToGroup(const IndexRange& entry, const int id);
    void removeEntryFromGroup(const IndexRange& entry, const int id);
    void addGroupXToY(const int X, const int Y);
    void removeGroupXFromY(const int X, const int Y);

    void addBoxes(const std::vector<BezierBox>& boxes, const std::vector<bool>& selection = std::vector<bool>());

    void clearAll();

    void draw();

    // returns false if no changes were made
    bool addMouseSelectionToGroup(const int id);

    // reach shaders
    GLProgram& program() { return m_program; }

    std::vector<size_t> getGroupIDs(int ID) const;
    // get a tuple containing the data needed for a camera to orbit around this group
    // first element is the point around which the camera will orbit, and second is the distance
    std::tuple<glm::vec3, float> getGroupOrbitData(int ID) const;
};