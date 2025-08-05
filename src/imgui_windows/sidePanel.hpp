#pragma once

#include "imguiWindow.hpp"
#include "controller.hpp"
#include "infoExchange.hpp"

class sidePanel : public imguiWindow{
private:
    infoExchange* channel; // since this panel will display data from this struct, its best to include it
    
    float m_width; // side panel width
public:
    sidePanel(infoExchange* c, const float w) : channel(c), m_width(w) {}
    void draw() override;
};