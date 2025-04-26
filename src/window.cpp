#include "window.hpp"

void Window::manageInputs()
{
    glfwPollEvents(); // poll inputs

    if (glfwGetKey(m_window, GLFW_KEY_R) == GLFW_PRESS)
        m_cam->setPosition(glm::vec3(0.f, 0.f, 0.f));
    
    bool panUp = false, panLeft = false, panDown = false, panRight = false;
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) panUp = true;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) panLeft = true;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) panDown = true;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) panRight = true;
    if (panUp || panLeft || panDown || panRight)
        m_cam->pan(panUp, panLeft, panDown, panRight);

    bool zoomIn = false, zoomOut = false;
    if (glfwGetKey(m_window, GLFW_KEY_I) == GLFW_PRESS) zoomIn = true;
    if (glfwGetKey(m_window, GLFW_KEY_O) == GLFW_PRESS) zoomOut = true;
    if (zoomIn || zoomOut)
        m_cam->zoom(zoomIn, zoomOut);
}

void Window::drawStuff()
{
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // we try to lock the s_drawables and draw them (since the vector is shared between threads)
    // this will work always, except when the file controller is writing to s_drawables, which is rare
    std::unique_lock lk(*s_drawables_mutex, std::try_to_lock);
    if (lk.owns_lock()){
        for(auto& dr:*s_drawables){
            if (!dr->getDidInit()) dr->initBuffers(); // if drawables havent been initialized, initialize
            dr->draw();
        }
        lk.unlock(); // release the lock early so it can be used by other threads
    }
    else{ // if the lock isnt available, it means that graph data is being loaded
        ImGui::TextDisabled("Loading..."); // TODO: add better loading, this one will create a window titled debug with the text "loading"
    }
    
    for(auto& win:m_imguis) win->draw(); // draw all imgui windows
    // ImGui::ShowDemoWindow();
    
    // render imgui things
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

Window::Window(int W, int H) : m_w_width(W), m_w_height(H)
{
    spdlog::info("GLFW version: {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);
    spdlog::info("GLEW version: {}.{}.{}", GLEW_VERSION_MAJOR, GLEW_VERSION_MINOR, GLEW_VERSION_MICRO);
    spdlog::info("IMGUI version: {}", IMGUI_VERSION);

    // init glfw library
    if (!glfwInit()){
        spdlog::error("GLFW init failed!");
        throw std::runtime_error("GLFW init failed!");
    }

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
        spdlog::error("GLEW init failed:\n{}", (char*)glewGetErrorString(err));
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error("GLEW init failed (check logs)");
    }

    // this requires GL version 4.3+
#ifdef DEBUG_LOGGING
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // ensures callbacks are synchronous
    glDebugMessageCallback(
        [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam){
            spdlog::warn("[OpenGL Debug]: {}", message);
    }, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

    glEnable(GL_CULL_FACE); // dont draw the side of a vertex that cant be seen
    glCullFace(GL_BACK); // the back side cant be seen
    glFrontFace(GL_CCW); // front is where the points of a triangle are connected counterclockwise

    // loging version info
    spdlog::info("OPENGL version: {}", (char*)glGetString(GL_VERSION));
    spdlog::info("Vendor: {}", (char*)glGetString(GL_VENDOR));
    spdlog::info("Renderer name: {}", (char*)glGetString(GL_RENDERER));

    // set icon
    glfwSetWindowIcon(m_window, 0, NULL); // TODO: make an icon

    // TODO: add an iconification callback which prevents window from being updated when active
    
    // initialize imgui
    IMGUI_CHECKVERSION(); // check that version is compatible with what its used for
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends for imgui
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330"); //glsl version
    
    // add custom imgui windows
    m_imguis.emplace_back(std::make_unique<menuBar>());
    m_imguis.emplace_back(std::make_unique<sidePanel>(300.f));

    // create shared variables on the heap
    s_drawables = std::make_shared<std::vector<std::unique_ptr<drawable>>>();
    s_drawables_mutex = std::make_shared<std::mutex>();

    // exchange shared data with the file controller thread
    tasks::addFileControllerDrawables(s_drawables, s_drawables_mutex);
    tasks::addFileControllerTask(tasks::CT_SET_DRAWABLES);
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
    // create the camera object
    m_cam = std::make_unique<Camera>(glm::vec3(0.f, 0.f, -1.f));

    // ===== CALLBACKS =====

    // create a structure for data passthrough in callbacks
    struct CallbackData {
        int* p_w_width;
        int* p_w_height;
        int* p_fb_width;
        int* p_fb_height;
        bool* cam_recalc;
    } callbackData;
    callbackData.p_w_width = &m_w_width;
    callbackData.p_w_height = &m_w_height;
    callbackData.p_fb_width = &m_fb_width;
    callbackData.p_fb_height = &m_fb_height;
    callbackData.cam_recalc = &m_cam->getRecalc();
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
        glViewport(0, 0, width, height);
        *data->cam_recalc = true;
    });

    // ==========

    // load the shader
    GLProgram program;

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
        this->manageInputs();

        // we should update the camera before drawing;
        m_cam->update(m_fb_width, m_fb_height);

        // pass the camera view matrix through uniform
        program.setUniformMat4f("MVP", m_cam->getMat());

        this->drawStuff();

        glfwSwapBuffers(m_window);
        // TODO: consider using glfwSwapInterval
        glFlush();
    }
    spdlog::info("Window Closed, notifiyng file controller to close...");
    tasks::addFileControllerTask(tasks::CT_EXIT);
}