target_sources(${PROJECT_NAME} PUBLIC
    "main.cpp"
    "window.cpp"
    "controller.cpp"
    "imgui_windows/menuBar.cpp"
    "imgui_windows/sidePanel.cpp"
    "gfa/GFA.cpp"
    "gfa/GFA_segment.cpp"
    "gfa/GFA_path.cpp"
    "graphics/quad.cpp"
    "graphics/GLProgram.cpp"
    "graphics/camera.cpp"
    "graphics/line.cpp"
)

# add the precompiled header
target_precompile_headers(${PROJECT_NAME} PRIVATE "pch.hpp")

target_include_directories(${PROJECT_NAME} PUBLIC
    "imgui_windows"
    "graphics"
    "gfa"
    "."
)