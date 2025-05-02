#pragma once

#include "imguiWindow.hpp"
#include "controller.hpp"
#include "infoExchange.hpp"

class sidePanel : public imguiWindow{
private:
    infoExchange* channel; // since this panel will display data from this struct, its best to include it
    
    float m_width; // side panel width
    bool m_collapsed; // is it collapsed
public:
    sidePanel(infoExchange* c, const float w);
    void draw() override;
};