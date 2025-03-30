#pragma once

#include "imguiWindow.hpp"
#include "controller.hpp"

class backgroundWindow : public imguiWindow{
private:
    void drawMenu();
public:
    backgroundWindow() = default;
    void draw() override;
};