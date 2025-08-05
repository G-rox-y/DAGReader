#pragma once

#include "imguiWindow.hpp"
#include "infoExchange.hpp"

class info : public imguiWindow{
private:
    infoExchange* channel; // we need shared data (paths)
public:
    info(infoExchange* info) : channel(info) {}
    void draw() override;
};