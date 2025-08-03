target_sources(${PROJECT_NAME} PUBLIC
    "main.cpp"
    "window.cpp"
    "controller.cpp"
    "imgui_windows/menuBar.cpp"
    "imgui_windows/sidePanel.cpp"
    "imgui_windows/about.cpp"
    "gfa/GFA.cpp"
    "gfa/GFA_segment.cpp"
    "gfa/GFA_path.cpp"
    "graphics/GLProgram.cpp"
    "graphics/camera.cpp"
    "graphics/renderer.cpp"
    "graphing/GRIP.cpp"
)

# add the precompiled header
target_precompile_headers(${PROJECT_NAME} PRIVATE "pch.hpp")

target_include_directories(${PROJECT_NAME} PUBLIC
    "${PROJECT_BINARY_DIR}/src"
    "imgui_windows"
    "graphics"
    "graphing"
    "gfa"
    "."
)

# build the version string for defines
set(DAGReader_VERSION_STRING "${DAGReader_VERSION_MAJOR}.${DAGReader_VERSION_MINOR}")
if(DEFINED DAGReader_VERSION_MICRO)
  string(APPEND DAGReader_VERSION_STRING ".${DAGReader_VERSION_MICRO}")
endif()

# write defines to a file
configure_file("cmakevars.hpp.in" "${PROJECT_BINARY_DIR}/src/cmakevars.hpp")
