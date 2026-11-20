#include "window.hpp"
#include "Roboto_Medium.hpp"

#include "frameLimiter.cpp"

void Window::manageInputs()
{
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    auto keyDown = [this](int key) { return glfwGetKey(m_window, key) == GLFW_PRESS; };

    bool moveFast = keyDown(GLFW_KEY_LEFT_SHIFT);

    bool moveUp = keyDown(GLFW_KEY_SPACE), moveLeft = keyDown(GLFW_KEY_A), moveDown = keyDown(GLFW_KEY_C);
    bool moveRight = keyDown(GLFW_KEY_D), moveIn = keyDown(GLFW_KEY_W), moveOut = keyDown(GLFW_KEY_S); 
    
    if (moveUp || moveLeft || moveDown || moveRight || moveIn || moveOut)
        channel->cam->move(moveUp, moveLeft, moveDown, moveRight, moveIn, moveOut, moveFast);

    bool rotUp = keyDown(GLFW_KEY_UP), rotLeft = keyDown(GLFW_KEY_LEFT), rotDown = keyDown(GLFW_KEY_DOWN);
    bool rotRight = keyDown(GLFW_KEY_RIGHT), yawCw = keyDown(GLFW_KEY_E), yawCcw = keyDown(GLFW_KEY_Q);
    
    if (rotUp || rotLeft || rotDown || rotRight || yawCw || yawCcw)
        channel->cam->rotate(rotUp, rotLeft, rotDown, rotRight, yawCw, yawCcw, moveFast);

    bool scaleIn = keyDown(GLFW_KEY_I), scaleOut = keyDown(GLFW_KEY_O);
    
    if (scaleIn || scaleOut)
        channel->cam->scale(scaleIn, scaleOut);

    // Other keys added as callbacks       

    // mouse inverse projection stuff
    double mouse_xpos, mouse_ypos;
    glfwGetCursorPos(m_window, &mouse_xpos, &mouse_ypos);
    mouse_xpos = 2.0 * mouse_xpos / static_cast<double>(m_fb_width) - 1.0;
    mouse_ypos = -2.0 * mouse_ypos / static_cast<double>(m_fb_height) + 1.0;
    glm::mat4 invMVP = glm::inverse(channel->cam->getMat());
    glm::vec4 mouseNear = invMVP * glm::vec4(mouse_xpos, mouse_ypos, -1.0, 1.0);
    glm::vec4 mouseFar = invMVP * glm::vec4(mouse_xpos, mouse_ypos, 1.0, 1.0);
    mouseNear /= mouseNear.w; mouseFar /= mouseFar.w;
    channel->renderer->program().setUniformVec3f("MouseNear", glm::vec3(mouseNear));
    channel->renderer->program().setUniformVec3f("MouseFar", glm::vec3(mouseFar));
}

void Window::drawStuff()
{
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();

    // selection mode circle indicator
    if (channel->selection_mode.load() && !io.WantCaptureMouse)
        ImGui::GetForegroundDrawList()->AddCircleFilled(ImVec2(
                ImGui::GetMainViewport()->WorkPos.x + ImGui::GetMainViewport()->WorkSize.x * 0.5f,
                ImGui::GetMainViewport()->WorkPos.y + 20.0f), 10, IM_COL32(255, 100, 100, 200
        ));

    // Theme colors
    struct ThemeColor { ImGuiCol_ idx; ImVec4 dark; ImVec4 light; };
    static constexpr ThemeColor themeColors[] = {
        { ImGuiCol_WindowBg,      ImVec4(0.05f, 0.05f, 0.05f, 0.4f), ImVec4(0.8f, 0.8f, 0.8f, 0.4f) },
        { ImGuiCol_Header,        ImVec4(0.05f, 0.05f, 0.05f, 0.0f), ImVec4(0.8f, 0.8f, 0.8f, 0.0f) },
        { ImGuiCol_TableHeaderBg, ImVec4(0.05f, 0.05f, 0.05f, 0.4f), ImVec4(0.8f, 0.8f, 0.8f, 0.4f) },
    };

    const bool light = channel->light_mode.load();
    for (const auto& t : themeColors)
        ImGui::PushStyleColor(t.idx, light ? t.light : t.dark);

    // Common colors
    ImVec4 c0(1.f, 0.65f, 0.65f, 0.85f), c1(1.f, 0.65f, 0.65f, 0.7f);
    ImVec4 c2(1.f, 0.65f, 0.65f, 0.4f), c3(1.f, 0.65f, 0.65f, 0.2f);
    struct CommonColor { ImGuiCol_ idx; ImVec4 col; };
    static const CommonColor commonColors[] = { {ImGuiCol_HeaderHovered, c1},
        {ImGuiCol_HeaderActive, c2}, {ImGuiCol_Button, c1}, {ImGuiCol_ButtonHovered, c2},
        {ImGuiCol_ButtonActive, c3}, {ImGuiCol_CheckMark, c1}, {ImGuiCol_FrameBg, c3},
        {ImGuiCol_FrameBgActive, c1}, {ImGuiCol_FrameBgHovered, c2}, {ImGuiCol_SliderGrab, c1},
        {ImGuiCol_SliderGrabActive, c0}, {ImGuiCol_TitleBg, c1}, {ImGuiCol_TitleBgActive, c1},
    };
    for (const auto& c : commonColors)
        ImGui::PushStyleColor(c.idx, c.col);

    // Background color
    if (channel->update_window_vars.load()){
        if (channel->light_mode.load()) glClearColor(1.f, 1.f, 1.f, 1.f);
        else glClearColor(0.1f, 0.1f, 0.1f, 1.f);
        channel->update_window_vars.store(false);
    }

    channel->renderer->draw(); // draw the graph stuff

    for(auto& win:m_imguis) win->draw(); // draw all imgui windows
    ImGui::PopStyleColor(16);

    // render imgui things
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

namespace {
    template<typename... Ts>
    void addWindows(std::vector<std::unique_ptr<imguiWindow>>& vec, infoExchange* ch) {
        (vec.emplace_back(std::make_unique<Ts>(ch)), ...);
    }
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    // glfwWindowHintString(GLFW_WAYLAND_APP_ID , "DAGReader"); // switch from glew to glad for this to be available
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
    
    // disable hardware cursor
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
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
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST); // make sure that the things that are in the back dont get drawn in front
    glDepthMask(GL_TRUE);
    if (channel->MSAA_enabled.load()) 
        glEnable(GL_MULTISAMPLE); // turn on MSAA

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

    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;  // let X handle cursor

    static const ImWchar CRO_RANGES[] = {
        0x0020, 0x00FF,   // Basic Latin + Latin-1
        0x0100, 0x017F,   // Latin Extended-A (č ć š đ ž …)
        0
    };
    [[maybe_unused]] ImFont* f = io.Fonts->AddFontFromMemoryCompressedTTF(
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
    addWindows<menuBar, sidePanel, about, controls, info, selectionWindow, license>(m_imguis, channel);
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
    channel->cam = std::make_unique<Camera>();
    channel->renderer = std::make_unique<Renderer>();

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
        data->channel->cam->shouldRecalc();
    });

    // key callback
    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods){
        // check if ImGui wants keyboard input and if yes dont use it
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard) return;

        CallbackData* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
        if (key == GLFW_KEY_H && action == GLFW_RELEASE)
            data->channel->controls_window_toggled.store(!data->channel->controls_window_toggled.load());
        if (key == GLFW_KEY_X && action == GLFW_RELEASE)
            data->channel->selection_mode.store(!data->channel->selection_mode.load());
        if (key == GLFW_KEY_R && action == GLFW_RELEASE)
            data->channel->cam->resetView();
        if (key == GLFW_KEY_U && action == GLFW_RELEASE){
            data->channel->renderer->clearGroup(-3); // hardcoded this identity cuz lambda shit
            data->channel->addControllerTask(tasks::REFRESH_GRAPH);
        }
        if (key == GLFW_KEY_F && action == GLFW_RELEASE){
            if (auto res = data->channel->renderer->getGroupOrbitData(-3)){
                auto [pos, d] = res.value();
                data->channel->cam->engageOrbit(pos, d*2);
                data->channel->cam->disengageOrbit();
                data->channel->cam->shouldRecalc();
            }
            // ^ a hacky way to go about this, but i dont feel like implementing a function just for this
        }
    });

    // lmb callback
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods){
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
        if (ImGui::GetIO().WantCaptureMouse) return;

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            CallbackData* data = static_cast<CallbackData*>(glfwGetWindowUserPointer(window));
            if (data->channel->selection_mode.load())
                if(data->channel->renderer->addMouseSelectionToGroup(groups::SELECTION))
                    data->channel->addControllerTask(tasks::REFRESH_GRAPH);
        }
    });

    // ==========

    // fetch framebuffer dimensions
    glfwGetFramebufferSize(m_window, &m_fb_width, &m_fb_height);

    // reveal the window (the window is hidden in the beginning to avoid showing the window while its loading)
    glfwShowWindow(m_window);
    glfwFocusWindow(m_window);
    spdlog::debug("Starting the window loop");
    FrameLimiter limiter(channel->max_fps.load());
    while(!glfwWindowShouldClose(m_window)) // window is running
    {
        limiter.changeFPS(channel->max_fps.load());
        glfwPollEvents();

        // clear the buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // system events
        this->manageInputs();

        // we should update the camera before drawing;
        channel->cam->update(m_fb_width, m_fb_height);

        // pass the camera view matrix through uniform
        channel->renderer->program().setUniformMat4f("MVP", channel->cam->getMat());
        channel->renderer->program().setUniform1i("Selection", channel->selection_mode.load());
        channel->renderer->program().setUniformVec3f("CamPos", channel->cam->getPos());
        channel->renderer->program().setUniform1f("PxPerRad", (float)m_fb_height / channel->cam->getFOV());

        // which faces to cull (whats the front and whats the back)
        bool mirrored = glm::determinant(channel->cam->getMat()) < 0.0f;
        glFrontFace(mirrored ? GL_CW : GL_CCW);

        this->drawStuff();

        glfwSwapBuffers(m_window);
        glFlush();

        // Frame limiter: sleep until the target frame time has elapsed
        limiter.wait();
    }
    spdlog::debug("Window Closed, notifiyng file controller to close...");
    channel->addControllerTask(tasks::EXIT);
}