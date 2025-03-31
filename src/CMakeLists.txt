target_sources(${PROJECT_NAME} PUBLIC
    "main.cpp"
    "window.cpp"
    "controller.cpp"
    "imgui_windows/menuBar.cpp"
    "imgui_windows/sidePanel.cpp"
    "gfa/GFA.cpp"
    "gfa/GFA_segment.cpp"
    "gfa/GFA_path.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "imgui_windows"
    "gfa"
    "."
)