#pragma once

#include "imguiWindow.hpp"
#include "controller.hpp"
#include "infoExchange.hpp"

class menuBar : public imguiWindow{
private:
    infoExchange* channel; // we need shared data (paths)
public:
    menuBar(infoExchange* c);
    void draw() override;
};