#pragma once

#include "imguiWindow.hpp"
#include "controller.hpp"

class sidePanel : public imguiWindow{
private:
    float m_width; // side panel width
    bool m_collapsed; // is it scollapsed
public:
    sidePanel(const float w);
    void draw() override;
};