#pragma once

#include "imguiWindow.hpp"
#include "infoExchange.hpp"

extern const char* licenseText;

class license : public imguiWindow{
private:
    infoExchange* channel;

    void setOffset(const std::string& text, float scale/* = 1.f*/);
public:
    license(infoExchange* info) : channel(info) {}
    void draw() override;
};