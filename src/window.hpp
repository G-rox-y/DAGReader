#pragma once

#include "imguiIncludes.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "pch.hpp"

#include "imguiWindow.hpp"
#include "menuBar.hpp"
#include "sidePanel.hpp"
#include "quad.hpp"
#include "GLProgram.hpp"
#include "camera.hpp"

// the class that controls the graphical window of the program
class Window{
private:
    GLFWwindow* m_window; // window pointer
    
    std::vector<std::unique_ptr<imguiWindow>> m_imguis; // list of imgui windows

    int m_w_width, m_w_height; // window width and height
    int m_fb_width, m_fb_height; // framebuffer width and height

    std::vector<Quad> m_quads; // make this a vector of drawables

    std::unique_ptr<Camera> m_cam;

    void manageInputs();
    void drawStuff();
public:
    Window(int W, int H);
    ~Window();

    void run();
};