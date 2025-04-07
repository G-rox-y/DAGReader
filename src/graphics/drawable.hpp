#pragma once

// abstract class for imgui windows
class drawable{
protected:
    bool m_didInit;
public:
    drawable(bool init) : m_didInit(init) {}
    virtual ~drawable() = default;
    virtual void draw() const = 0;
    virtual void initBuffers() = 0;

    bool& getDidInit() { return m_didInit; }
};