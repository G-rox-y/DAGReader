#pragma once

#include "imguiWindow.hpp"
#include "controller.hpp"
#include "infoExchange.hpp"

class sidePanel : public imguiWindow{
private:
    infoExchange* channel; // since this panel will display data from this struct, its best to include it

    // appearance state
    std::array<float, 4> m_segColors{0,0,0,0};
    std::array<float, 4> m_linkColors{0,0,0,0};
    std::array<float, 4> m_selectionColors{0,0,0,0};
    int m_segColorScheme = 0;
    int m_linkColorScheme = 0;
    int m_segColorRule = 0;

    // data
    std::map<std::string, dataProperties> data;

    // helper functions
    void drawLayout(float availX, float w);
    void drawAppearance(float availX, float w);
    void drawSubgraphSelection(float availX, float w, size_t subgraphNum);
    void drawSearch(float availX, float w);
public:
    sidePanel(infoExchange* c);
    void draw() override;
};