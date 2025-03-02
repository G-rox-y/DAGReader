#pragma once

#include "imguiIncludes.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imguiWindow.hpp"

#include <vector>
#include <memory>

// the class that controls the graphical window of the program
class Window{
private:
    GLFWwindow* m_window; // window pointer

    std::vector<std::unique_ptr<imguiWindow>> imguis; // list of imgui windows

    int m_w_width, m_w_height; // window width and height
    int m_fb_width, m_fb_height; // framebuffer width and height

    void update();
public:
    Window(int W, int H);
    ~Window();

    void run();
};