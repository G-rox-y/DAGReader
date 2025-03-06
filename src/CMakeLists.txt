target_sources(${PROJECT_NAME} PUBLIC
    "main.cpp"
    "window.cpp"
    "imgui_windows/backgroundWindow.cpp"
    "gfa/GFA.cpp"
    "gfa/GFA_link.cpp"
    "gfa/GFA_segment.cpp"
    "gfa/GFA_containment.cpp"
    "gfa/GFA_path.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "imgui_windows"
    "gfa"
    "."
)