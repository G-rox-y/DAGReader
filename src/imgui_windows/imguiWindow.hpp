#pragma once
#include "imguiIncludes.hpp"

// abstract class for imgui windows
class imguiWindow{
public:
    virtual ~imguiWindow() = default;
    virtual void draw() = 0;
};