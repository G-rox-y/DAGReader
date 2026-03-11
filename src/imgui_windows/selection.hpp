#pragma once

#include "imguiWindow.hpp"
#include "infoExchange.hpp"

class selectionWindow : public imguiWindow{
private:
    infoExchange* channel;
    bool m_verboseMode = false;
public:
    selectionWindow(infoExchange* c) : channel(c) {}
    void draw() override;
};