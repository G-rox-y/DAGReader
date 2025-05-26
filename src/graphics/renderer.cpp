#include "renderer.hpp"


Renderer::Renderer()
{
    glGenVertexArrays(1, &mt_VA);
    glGenBuffers(1, &mt_VB);
    glGenBuffers(1, &mt_EB);

    glGenVertexArrays(1, &ml_VA);
    glGenBuffers(1, &ml_VB);
    glGenBuffers(1, &ml_EB);
}

Renderer::~Renderer()
{
    glDeleteBuffers(1, &ml_EB);
    glDeleteBuffers(1, &ml_VB);
    glDeleteVertexArrays(1, &ml_VA);
    
    glDeleteBuffers(1, &mt_EB);
    glDeleteBuffers(1, &mt_VB);
    glDeleteVertexArrays(1, &mt_VA);
}

void Renderer::addQuad(glm::vec2 center, float length, float width, float angle, glm::u8vec4 color)
{
    // points of the quad
    glm::vec2 p1(center.x - length, center.y - width), p2(center.x + length, center.y - width),
        p3(center.x + length, center.y + width), p4(center.x - length, center.y + width);

    // now you should have them rotated
    glm::vec2 p1_rot = glm::rotate(p1 - center, angle) + center, p2_rot = glm::rotate(p2 - center, angle) + center,
        p3_rot = glm::rotate(p3 - center, angle) + center, p4_rot = glm::rotate(p4 - center, angle) + center;

    std::lock_guard lk(mem_mut);
    
    // fill the index array
    int b = mt_ivmem.size() / 3;
    mt_iimem.insert(mt_iimem.end(), {
        b+0, b+1, b+2,
        b+2, b+3, b+0
    });

    // and fill the array
    mt_ivmem.insert(mt_ivmem.end(), {
        p1_rot.x, p1_rot.y, 0.f,
        p2_rot.x, p2_rot.y, 0.f,
        p3_rot.x, p3_rot.y, 0.f,
        p4_rot.x, p4_rot.y, 0.f
    });

    mt_ivcmem.insert(mt_ivcmem.end(), {color, color, color, color}); // 4 colors 4 points
}

void Renderer::addLine(const std::vector<float>& pts, glm::u8vec4 color)
{
    std::lock_guard lk(mem_mut);

    if (pts.size() < 4) return;
    else if (pts.size() == 4){
        ml_vmem.insert(ml_vmem.end(), pts.begin(), pts.end());
        ml_vcmem.insert(ml_vcmem.end(), {color, color}); // 2 pts
        return;
    }

    int b = ml_ivmem.size() / 2;
    for(size_t i = 1; i < pts.size()/2; i++){
        ml_iimem.insert(ml_iimem.begin(), {b+(int)i-1, b+(int)i});
        ml_ivcmem.push_back(color);
    }
    
    ml_ivmem.insert(ml_ivmem.end(), pts.begin(), pts.end());
}

void Renderer::addBox(glm::vec3 begin, glm::vec3 beginNormal, glm::vec3 end, glm::vec3 endNormal, glm::u8vec4 color)
{
    // lambda for creating planes
    auto makePlane = [this](glm::vec3 center, glm::vec3 normal, glm::u8vec4 color){
        float halfH = 0.1f, halfW = 0.1f;

        glm::vec3  N = glm::normalize(normal);
        glm::vec3  up = glm::abs(N.y) < 0.999f ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
        glm::vec3  X = glm::normalize(glm::cross(up, N));
        glm::vec3  Y = glm::cross(N, X); 

        glm::vec3 c1 = center + (+halfW * X) + (-halfH * Y);
        glm::vec3 c2 = center + (+halfW * X) + (+halfH * Y);
        glm::vec3 c3 = center + (-halfW * X) + (+halfH * Y);
        glm::vec3 c4 = center + (-halfW * X) + (-halfH * Y);
        
        int b = mt_ivmem.size() / 3;
        mt_iimem.insert(mt_iimem.end(), {
            b+0, b+1, b+2,
            b+2, b+3, b+0,
        });

        mt_ivmem.insert(mt_ivmem.end(), {
            c1.x, c1.y, c1.z,
            c2.x, c2.y, c2.z,
            c3.x, c3.y, c3.z,
            c4.x, c4.y, c4.z
        });

        mt_ivcmem.insert(mt_ivcmem.end(), {color, color, color, color});
    };

    // save the position before adding vertices for more intuitive index handling
    int b = mt_ivmem.size() / 3;

    // create planes
    makePlane(begin, beginNormal, color);
    makePlane(end, endNormal, color);
    
    // connect planes
    mt_iimem.insert(mt_iimem.end(), {
        b+0, b+1, b+4,
        b+1, b+5, b+4,
        b+1, b+2, b+5,
        b+2, b+6, b+5,
        b+2, b+3, b+6,
        b+3, b+7, b+6,
        b+3, b+0, b+7,
        b+0, b+4, b+7
    });
}

void Renderer::clearAll()
{
    std::lock_guard lk(mem_mut);

    mt_ivmem.clear(); mt_vmem.clear(); mt_iimem.clear(); mt_ivcmem.clear(); mt_vcmem.clear();
    mt_indexedSize = mt_unindexedSize = mt_indexSize = 0;

    ml_ivmem.clear(); ml_vmem.clear(); ml_iimem.clear(); ml_ivcmem.clear(); ml_vcmem.clear();
    ml_indexedSize = ml_unindexedSize = ml_indexSize = 0;

    m_shouldUpdate.store(true);
}

void Renderer::updateBuffers()
{
    if (!m_shouldUpdate.load()) return;
    
    std::lock_guard lk(mem_mut);

    // for combining vertex data
    std::vector<std::byte> combined;
    auto combinedAppend = [&combined](auto const& vec){
        using T = typename std::decay_t<decltype(vec)>::value_type;
        combined.insert(combined.end(),
            reinterpret_cast<const std::byte*>(vec.data()),
            reinterpret_cast<const std::byte*>(vec.data()) + vec.size()*sizeof(T)
        );
    };

    // first the triangles

    // combine vertex data
    size_t vsize = (mt_ivmem.size() + mt_vmem.size()) * sizeof(float), csize = (mt_ivcmem.size() + mt_vcmem.size()) * sizeof(glm::u8vec4);
    combined.reserve(vsize + csize);
    combinedAppend(mt_ivmem);
    combinedAppend(mt_vmem);
    combinedAppend(mt_ivcmem);
    combinedAppend(mt_vcmem);

    // bind the vertex array
    glBindVertexArray(mt_VA);

    // fill the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, mt_VB);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(combined.size()), combined.data(), GL_STATIC_DRAW);

    // set the vertex layout
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(glm::u8vec4), (void*)vsize);
    glEnableVertexAttribArray(1);

    // fill the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mt_EB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mt_iimem.size() * sizeof(int), mt_iimem.data(), GL_STATIC_DRAW); 

    mt_indexSize = mt_iimem.size();
    mt_indexedSize = mt_ivmem.size() / 3;
    mt_unindexedSize = mt_vmem.size() / 3;

    // now the lines
    combined.clear();
    vsize = (ml_ivmem.size() + ml_vmem.size()) * sizeof(float), csize = (ml_ivcmem.size() + ml_vcmem.size()) * sizeof(glm::u8vec4);
    combined.reserve(vsize + csize);
    combinedAppend(ml_ivmem);
    combinedAppend(ml_vmem);
    combinedAppend(ml_ivcmem);
    combinedAppend(ml_vcmem);

    glBindVertexArray(ml_VA);

    glBindBuffer(GL_ARRAY_BUFFER, ml_VB);
    glBufferData(GL_ARRAY_BUFFER, combined.size() * sizeof(float), combined.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(glm::u8vec4), (void*)vsize);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ml_EB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ml_iimem.size() * sizeof(int), ml_iimem.data(), GL_STATIC_DRAW); 

    ml_indexSize = ml_iimem.size();
    ml_indexedSize = ml_ivmem.size() / 2;
    ml_unindexedSize = ml_vmem.size() / 2;

    m_shouldUpdate.store(false);
}

void Renderer::draw()
{
    updateBuffers();
    glBindVertexArray(mt_VA);
    glDrawElements(GL_TRIANGLES, mt_indexSize, GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_TRIANGLES, mt_indexedSize, mt_unindexedSize);

    glBindVertexArray(ml_VA);
    glDrawElements(GL_LINES, ml_indexSize, GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_LINES, ml_indexedSize, ml_unindexedSize);
}