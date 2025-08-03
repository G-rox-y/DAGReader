#pragma once

#include "imguiIncludes.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "pch.hpp"

#include "imguiWindow.hpp"
#include "menuBar.hpp"
#include "sidePanel.hpp"
#include "about.hpp"
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

    // TODO: for now quads do draw calls individually, this should be made into a batch

    std::shared_ptr<Camera> m_cam;
    std::shared_ptr<Renderer> m_renderer;

    void manageInputs();
    void drawStuff();
public:
    Window(infoExchange* c, int W, int H);
    ~Window();

    void run();
};