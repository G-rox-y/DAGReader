#pragma once

#include "imguiWindow.hpp"
#include "infoExchange.hpp"
#include "sequenceWindow.hpp"
#include "gfa/GFA.hpp"

class selectionWindow : public imguiWindow{
private:
    infoExchange* channel;
    bool m_verboseMode = false;

    sequenceWindow seqWin;

    float m_customColor[4] = {1.0f, 0.2f, 0.2f, 1.0f}; // default paint color
public:
    selectionWindow(infoExchange* c) : channel(c), seqWin(c) {}
    void draw() override;
};