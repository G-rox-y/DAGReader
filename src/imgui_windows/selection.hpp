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
public:
    selectionWindow(infoExchange* c) : channel(c), seqWin(c) {}
    void draw() override;
};