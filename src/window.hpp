#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// the class that controls the graphical window of the program
class Window{
private:
    GLFWwindow* m_window; // window pointer

    int m_w_width, m_w_height; // window width and height
    int m_fb_width, m_fb_height; // framebuffer width and height

    void update();
public:
    Window(int W, int H);
    ~Window();

    void run();
};