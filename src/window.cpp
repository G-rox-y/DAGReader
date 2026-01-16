#include "window.hpp"
#include "Roboto_Medium.hpp"

void Window::manageInputs()
{
    glfwPollEvents(); // poll inputs

    bool moveUp = false, moveLeft = false, moveDown = false, moveRight = false, moveIn = false, moveOut = false, moveFast = false;
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) moveUp = true;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) moveLeft = true;
    if (glfwGetKey(m_window, GLFW_KEY_C) == GLFW_PRESS) moveDown = true;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) moveRight = true;
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) moveIn = true;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) moveOut = true;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) moveFast = true;
    if (moveUp || moveLeft || moveDown || moveRight || moveIn || moveOut)
        m_cam->move(moveUp, moveLeft, moveDown, moveRight, moveIn, moveOut, moveFast);

    bool rotUp = false, rotLeft = false, rotDown = false, rotRight = false, yawCw = false, yawCcw = false;
    if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS) rotUp = true;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS) rotLeft = true;
    if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS) rotDown = true;
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS) rotRight = true;
    if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS) yawCw = true;
    if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS) yawCcw = true;
    if (rotUp || rotLeft || rotDown || rotRight || yawCw || yawCcw)
        m_cam->rotate(rotUp, rotLeft, rotDown, rotRight, yawCw, yawCcw, moveFast);

    bool scaleIn = false, scaleOut = false;
    if (glfwGetKey(m_window, GLFW_KEY_I) == GLFW_PRESS) scaleIn = true;
    if (glfwGetKey(m_window, GLFW_KEY_O) == GLFW_PRESS) scaleOut = true;
    if (scaleIn || scaleOut)
        m_cam->scale(scaleIn, scaleOut);

    // Other keys added as callbacks       

    // mouse inverse projection stuff
    double mouse_xpos, mouse_ypos;
    glfwGetCursorPos(m_window, &mouse_xpos, &mouse_ypos);
    mouse_xpos = 2.0 * mouse_xpos / static_cast<double>(m_fb_width) - 1.0;
    mouse_ypos = -2.0 * mouse_ypos / static_cast<double>(m_fb_height) + 1.0;
    glm::mat4 invMVP = glm::inverse(m_cam->getMat());
    glm::vec4 mouseNear = invMVP * glm::vec4(mouse_xpos, mouse_ypos, -1.0, 1.0);
    glm::vec4 mouseFar = invMVP * glm::vec4(mouse_xpos, mouse_ypos, 1.0, 1.0);
    mouseNear /= mouseNear.w; mouseFar /= mouseFar.w;
    m_renderer->program.setUniformVec3f("MouseNear", glm::vec3(mouseNear));
    m_renderer->program.setUniformVec3f("MouseFar", glm::vec3(mouseFar));
}

void Window::drawStuff()
{
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();

    if (channel->selection_mode.load() && !io.WantCaptureMouse)
        ImGui::SetMouseCursor(7);

    // colors
    ImVec4 c0(1.f, 0.65f, 0.65f, 0.85f);
    ImVec4 c1(1.f, 0.65f, 0.65f, 0.7f);
    ImVec4 c2(1.f, 0.65f, 0.65f, 0.4f);
    ImVec4 c3(1.f, 0.65f, 0.65f, 0.2f);

    if (channel->light_mode.load()){
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.8f, 0.8f, 0.8f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.8f, 0.8f, 0.8f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,  ImVec4(0.8f, 0.8f, 0.8f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,  c1);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, c2);
        ImGui::PushStyleColor(ImGuiCol_Button,  c1);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  c2);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  c3);
        ImGui::PushStyleColor(ImGuiCol_CheckMark,  c1);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,  c3);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  c1);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,  c2);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab,  c1);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,  c0);
        ImGui::PushStyleColor(ImGuiCol_TitleBg,  c1);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive,  c1);
    }
    else{
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.05f, 0.05f, 0.05f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,  ImVec4(0.05f, 0.05f, 0.05f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, c1);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, c2);
        ImGui::PushStyleColor(ImGuiCol_Button, c1);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c2);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, c3);
        ImGui::PushStyleColor(ImGuiCol_CheckMark, c1);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, c3);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, c1);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, c2);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, c1);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, c0);
        ImGui::PushStyleColor(ImGuiCol_TitleBg, c1);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, c1);
    }

    // set background color
    if (channel->update_window_vars.load()){
        if (channel->light_mode.load()) glClearColor(1.f, 1.f, 1.f, 1.f);
        else glClearColor(0.1f, 0.1f, 0.1f, 1.f);
        channel->update_window_vars.store(false);
    }

    m_renderer->draw();

    for(auto& win:m_imguis) win->draw(); // draw all imgui windows
    ImGui::PopStyleColor(16);

    //ImGui::ShowDemoWindow(); // will compile only if build type is debug
    
    // render imgui things
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

Window::Window(infoExchange* c, int W, int H) : channel(c), m_w_width(W), m_w_height(H)
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

    glfwWindowHint(GLFW_SAMPLES, 4); // request 4×MSAA

    // create the window
    m_window = glfwCreateWindow(m_w_width, m_w_height, "DAGReader", NULL, NULL);
    if (!m_window) [[unlikely]] {
        spdlog::error("Window init failed!");
        glfwTerminate();
        throw std::runtime_error("Window init failed!");
    }
    
    // activate the context
    glfwMakeContextCurrent(m_window);
    
    // load opengl functions using glew
    glewExperimental = true;
    GLenum err = glewInit();
    if(err != GLEW_OK) [[unlikely]] {
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
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#endif

    glEnable(GL_CULL_FACE); // dont draw the side of a vertex that cant be seen
    glCullFace(GL_BACK); // the back side cant be seen
    glFrontFace(GL_CCW); // front is where the points of a triangle are connected counterclockwise
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST); // make sure that the things that are in the back dont get drawn in front
    glDepthMask(GL_TRUE);
    glEnable(GL_MULTISAMPLE); // turn on MSAA
    // TODO: ADD option to disable MSAA

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
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();

    static const ImWchar CRO_RANGES[] = {
        0x0020, 0x00FF,   // Basic Latin + Latin-1
        0x0100, 0x017F,   // Latin Extended-A (č ć š đ ž …)
        0
    };
    ImFont* f = io.Fonts->AddFontFromMemoryCompressedTTF(
        Roboto_Medium_compressed_data, Roboto_Medium_compressed_size, 15.0f, nullptr, CRO_RANGES
    );
    IM_ASSERT(f && "Font failed to load");
    
    // Setup Platform/Renderer backends for imgui
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
#ifdef NO_GL_4_3
    ImGui_ImplOpenGL3_Init("#version 330"); //glsl version
#else
    ImGui_ImplOpenGL3_Init("#version 430"); //glsl version
#endif
    // add custom imgui windows
    m_imguis.emplace_back(std::make_unique<menuBar>(channel));
    m_imguis.emplace_back(std::make_unique<sidePanel>(channel));
    m_imguis.emplace_back(std::make_unique<about>(channel));
    m_imguis.emplace_back(std::make_unique<controls>(channel));
    m_imguis.emplace_back(std::make_unique<info>(channel));
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
    channel->cam = m_cam = std::make_shared<Camera>();
    channel->renderer = m_renderer = std::make_shared<Renderer>();

    // ===== CALLBACKS =====

    // create a structure for data passthrough in callbacks
    struct CallbackData {
        int* p_w_width;
        int* p_w_height;
        int* p_fb_width;
        int* p_fb_height;
        bool* cam_recalc;
        infoExchange* channel;
    } callbackData;
    callbackData.p_w_width = &m_w_width;
    callbackData.p_w_height = &m_w_height;
    callbackData.p_fb_width = &m_fb_width;
    callbackData.p_fb_height = &m_fb_height;
    callbackData.cam_recalc = &m_cam->getRecalc();
    callbackData.channel = channel;
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

    // key callback
    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods){
        if (key == GLFW_KEY_H && action == GLFW_RELEASE) {
            CallbackData* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
            data->channel->controls_window_shown.store(!data->channel->controls_window_shown.load());
        }
        if (key == GLFW_KEY_X && action == GLFW_RELEASE) {
            CallbackData* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
            data->channel->selection_mode.store(!data->channel->selection_mode.load());
        }
        if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
            CallbackData* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
            data->channel->cam->resetView();
        }
    });

    // lmb callback
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods){
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            CallbackData* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
            if (data->channel->selection_mode.load()){
                int id = data->channel->renderer->getNearestToMouse();
                auto entry = std::make_pair(id, id);
                if (id != -1){
                    if (data->channel->renderer->isEntryInGroup(entry, -3))
                        data->channel->renderer->removeEntryFromGroup(entry, -3);
                    else data->channel->renderer->addEntryToGroup(entry, -3);

                    data->channel->addControllerTask(tasks::REFRESH_GRAPH);
                }
            }
        }
    });

    // ==========

    // fetch framebuffer dimensions
    glfwGetFramebufferSize(m_window, &m_fb_width, &m_fb_height);

    // reveal the window (the window is hidden in the beginning to avoid showing the window while its loading)
    glfwShowWindow(m_window);
    glfwFocusWindow(m_window);
    spdlog::debug("Starting the window loop");
    while(!glfwWindowShouldClose(m_window)) // window is running
    {
        // clear the buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // system events
        this->manageInputs();

        // we should update the camera before drawing;
        m_cam->update(m_fb_width, m_fb_height);

        // pass the camera view matrix through uniform
        m_renderer->program.setUniformMat4f("MVP", m_cam->getMat());
        m_renderer->program.setUniform1f("Scale", m_cam->getScale());
        m_renderer->program.setUniform1i("Selection", channel->selection_mode.load());
        m_renderer->program.setUniformVec3f("CamPos", m_cam->getPos());
        m_renderer->program.setUniform1f("PxPerRad", (float)m_fb_height / m_cam->getFOV());

        // which faces to cull (whats the front and whats the back)
        bool mirrored = glm::determinant(m_cam->getMat()) < 0.0f;
        glFrontFace(mirrored ? GL_CW : GL_CCW);

        this->drawStuff();

        glfwSwapBuffers(m_window);
        // TODO: consider using glfwSwapInterval
        glFlush();
    }
    spdlog::debug("Window Closed, notifiyng file controller to close...");
    channel->addControllerTask(tasks::EXIT);
}