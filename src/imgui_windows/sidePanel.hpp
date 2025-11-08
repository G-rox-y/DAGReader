#pragma once

#include "imguiWindow.hpp"
#include "controller.hpp"
#include "infoExchange.hpp"

class sidePanel : public imguiWindow{
private:
    infoExchange* channel; // since this panel will display data from this struct, its best to include it
public:
    sidePanel(infoExchange* c) : channel(c) {}
    void draw() override;
};