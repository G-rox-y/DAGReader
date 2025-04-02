#include "window.hpp"

void Window::update()
{
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    for(auto& q:quads) q.draw(); // draw quads
    for(auto& win:imguis) win->draw(); // draw all imgui windows
    //ImGui::ShowDemoWindow();   
    
    // render imgui things
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

Window::Window(int W, int H) : m_w_width(W), m_w_height(H)
{
    // init glfw library
    if (!glfwInit()){
        spdlog::error("GLFW init failed!");
        throw std::runtime_error("GLFW init failed!");
    }

    spdlog::info("GLFW version: {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);
    
    // add window hints
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    // glfwWindowHintString(GLFW_WAYLAND_APP_ID , "DAGReader"); // this seems to be unsupported rn
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "DAGReader");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "DAGReader");
    
    // create the window
    m_window = glfwCreateWindow(m_w_width, m_w_height, "DAGReader", NULL, NULL);
    if (!m_window){
        spdlog::error("Window init failed!");
        glfwTerminate();
        throw std::runtime_error("Window init failed!");
    }
    
    // activate the context
    glfwMakeContextCurrent(m_window);
    
    // load opengl functions using glew
    glewExperimental = true;
    GLenum err = glewInit();
    if(err != GLEW_OK){
        spdlog::error("Error: GLEW init failed:\n{}", (char*)glewGetErrorString(err));
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error("Error: GLEW init failed (check logs)");
    }

    // loging version info
    spdlog::info("OPENGL version: {}", (char*)glGetString(GL_VERSION));
    spdlog::info("Info: Vendor: {}", (char*)glGetString(GL_VENDOR));
    spdlog::info("Info: Renderer name: {}", (char*)glGetString(GL_RENDERER));

    // set icon
    glfwSetWindowIcon(m_window, 0, NULL); // TODO: make an icon

    // ===== CALLBACKS =====

    // create a structure for data passthrough in callbacks
    struct CallbackData {
        int* p_w_width;
        int* p_w_height;
        int* p_fb_width;
        int* p_fb_height;
    } callbackData;
    callbackData.p_w_width = &m_w_width;
    callbackData.p_w_height = &m_w_height;
    callbackData.p_fb_width = &m_fb_width;
    callbackData.p_fb_height = &m_fb_height;
    glfwSetWindowUserPointer(m_window, &callbackData);

    // set resize actions
    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height){
        CallbackData* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window)); // retrieve callback data
        *data->p_w_width = width;
        *data->p_w_height = height;
    });
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height){
        CallbackData* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window)); // retrieve callback data
        *data->p_fb_width = width;
        *data->p_fb_height = height;
    });

    // ==========

    // TODO: add an iconification callback which prevents window from being updated when active
    
    // initialize imgui
    IMGUI_CHECKVERSION(); // check that version is compatible with what its used for
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    ImGui::StyleColorsDark();
    
    spdlog::info("Info: IMGUI version: {}", IMGUI_VERSION);
    
    // Setup Platform/Renderer backends for imgui
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330"); //glsl version
    
    // add custom imgui windows
    imguis.emplace_back(std::make_unique<menuBar>());
    imguis.emplace_back(std::make_unique<sidePanel>(300.f));    
}

Window::~Window()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    glfwDestroyWindow(m_window);
    ImGui::DestroyContext();
    glfwTerminate();
}

void Window::run()
{
    // fetch framebuffer dimensions
    glfwGetFramebufferSize(m_window, &m_fb_width, &m_fb_height);

    // set background color
    glClearColor(0.f, 0.f, 0.f, 1.f);


    // reveal the window (the window is hidden in the beginning to avoid showing the window while its loading)
    glfwShowWindow(m_window);
    glfwFocusWindow(m_window);
    spdlog::info("Starting the window loop");
    while(!glfwWindowShouldClose(m_window)) // window is running
    {
        // clear the buffer
        glClear(GL_COLOR_BUFFER_BIT	| GL_DEPTH_BUFFER_BIT);

        // system events
        glfwPollEvents(); 

        this->update();

        glfwSwapBuffers(m_window);
        // TODO: consider using glfwSwapInterval
        glFlush();
    }
    spdlog::info("Window Closed");
}