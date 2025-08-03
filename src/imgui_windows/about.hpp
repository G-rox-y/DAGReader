#pragma once

#include "imguiWindow.hpp"
#include "infoExchange.hpp"

class about : public imguiWindow{
private:
    infoExchange* channel; // we need shared data (paths)
    bool m_collapsed = false;

    void setOffset(const std::string& text, float scale/* = 1.f*/);
public:
    about(infoExchange* info) : channel(info) {}
    void draw() override;
};