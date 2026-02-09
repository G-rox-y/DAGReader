#pragma once

#include "imguiIncludes.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "pch.hpp"

#include "imgui_windows/imguiWindow.hpp"
#include "imgui_windows/menuBar.hpp"
#include "imgui_windows/sidePanel.hpp"
#include "imgui_windows/about.hpp"
#include "imgui_windows/controls.hpp"
#include "imgui_windows/info.hpp"
#include "GLProgram.hpp"
#include "camera.hpp"
#include "renderer.hpp"
#include "infoExchange.hpp"

// the class that controls the graphical window of the program
class Window{
private:
    infoExchange* channel; // shared variables between threads

    GLFWwindow* m_window; // window pointer
    
    std::vector<std::unique_ptr<imguiWindow>> m_imguis; // list of imgui windows

    int m_w_width, m_w_height; // window width and height
    int m_fb_width, m_fb_height; // framebuffer width and height

    void manageInputs();
    void drawStuff();
public:
    Window(infoExchange* c, int W, int H);
    ~Window();

    void run();
};