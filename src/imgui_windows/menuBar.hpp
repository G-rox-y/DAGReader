#pragma once

#include "imguiWindow.hpp"
#include "controller.hpp"

class menuBar : public imguiWindow{
private:
public:
    menuBar() = default;
    void draw() override;
};