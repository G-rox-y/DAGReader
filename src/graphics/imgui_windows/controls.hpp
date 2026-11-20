#pragma once

#include "imguiWindow.hpp"
#include "infoExchange.hpp"

class controls : public imguiWindow{
private:
    infoExchange* channel; // we need shared data (paths)
    bool m_collapsed = false;
    float m_width = 0.f;
public:
    controls(infoExchange* info) : channel(info) {}
    void draw() override;
};