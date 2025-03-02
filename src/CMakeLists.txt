target_sources(${PROJECT_NAME} PUBLIC
    "main.cpp"
    "window.cpp"
    "imgui_windows/backgroundWindow.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "imgui_windows"
    "."
)