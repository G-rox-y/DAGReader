#pragma once

#include "imguiIncludes.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "pch.hpp"

#include "imguiWindow.hpp"
#include "menuBar.hpp"
#include "sidePanel.hpp"
#include "GLProgram.hpp"
#include "camera.hpp"

// the class that controls the graphical window of the program
class Window{
private:
    GLFWwindow* m_window; // window pointer
    
    std::vector<std::unique_ptr<imguiWindow>> m_imguis; // list of imgui windows

    int m_w_width, m_w_height; // window width and height
    int m_fb_width, m_fb_height; // framebuffer width and height

    // a vector of drawables for drawing on the heap
    // the file controller is going to be filling this vector, so the pointer to this object will be shared with another thread
    std::shared_ptr<std::vector<std::unique_ptr<drawable>>> s_drawables; // the s_ stands for shared instead of member as in m_
    // TODO: for now quads do draw calls individually, this should be made into a batch

    // due to vector sharing, we will also need a mutex
    std::shared_ptr<std::mutex> s_drawables_mutex;

    std::unique_ptr<Camera> m_cam;

    void manageInputs();
    void drawStuff();
public:
    Window(int W, int H);
    ~Window();

    void run();
};