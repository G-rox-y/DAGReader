#pragma once

// abstract class for imgui windows
class drawable{
public:
    virtual ~drawable() = default;
    virtual void draw() = 0;
};