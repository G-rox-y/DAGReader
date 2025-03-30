cmake_minimum_required(VERSION 3.24)

# project init
project(DAGReader VERSION 0.1)

# default build type is debug
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified. Defaulting to Debug.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# set important vars
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

# -------------------- DEPENDENCIES

include(FetchContent)

# add glfw
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 3.4
)

FetchContent_GetProperties(glfw)
if(NOT glfw_POPULATED)
    FetchContent_Populate(glfw)

    set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
    set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
    set(ONLY_LIBS ON CACHE INTERNAL "" FORCE)

    add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR})
endif()

# add glew-cmake
FetchContent_Declare(glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake
    GIT_TAG 9758219375d923a008feef93c0955931d801740f # master branch Jun 26 2023
)

FetchContent_GetProperties(glew)
if(NOT glew_POPULATED)
    FetchContent_Populate(glew)

    set(glew-cmake_BUILD_SHARED OFF CACHE INTERNAL "" FORCE)

    add_subdirectory(${glew_SOURCE_DIR} ${glew_BINARY_DIR})
endif()

# add glm
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# add imgui
FetchContent_Declare(imgui-docking
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.89.9-docking
)

FetchContent_GetProperties(imgui-docking)
if(NOT imgui-docking_POPULATED)
    FetchContent_Populate(imgui-docking)
endif()

# add nfd-extended
# TODO: test for dependencies and their availlability (same goes for other packages)
FetchContent_Declare(nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
    GIT_TAG v1.2.1
)

FetchContent_GetProperties(nfd)
if(NOT nfd_POPULATED)
    FetchContent_Populate(nfd)
endif()

# nfd is needed as a subdirectory
add_subdirectory("${PROJECT_BINARY_DIR}/_deps/nfd-src/")

# -------------------- END OF DEPENDENCIES

# create executable
add_executable(${PROJECT_NAME})

# add code
add_subdirectory("src")

# add imguis code since they dont have their own (useable) cmake
target_sources(${PROJECT_NAME} PUBLIC
    "${PROJECT_BINARY_DIR}/_deps/imgui-docking-src/misc/cpp/imgui_stdlib.cpp" # primarily for std::string
    "${PROJECT_BINARY_DIR}/_deps/imgui-docking-src/imgui.cpp"
    "${PROJECT_BINARY_DIR}/_deps/imgui-docking-src/imgui_draw.cpp"
    "${PROJECT_BINARY_DIR}/_deps/imgui-docking-src/imgui_demo.cpp"  # delete this one before moving to the main branch
    "${PROJECT_BINARY_DIR}/_deps/imgui-docking-src/imgui_tables.cpp"
    "${PROJECT_BINARY_DIR}/_deps/imgui-docking-src/imgui_widgets.cpp"
    "${PROJECT_BINARY_DIR}/_deps/imgui-docking-src/backends/imgui_impl_glfw.cpp"
    "${PROJECT_BINARY_DIR}/_deps/imgui-docking-src/backends/imgui_impl_opengl3.cpp"
)

# include dependencies
target_include_directories(${PROJECT_NAME} PUBLIC
    "${PROJECT_BINARY_DIR}/_deps/glm-src/"
    "${PROJECT_BINARY_DIR}/_deps/glew-src/include/"
    "${PROJECT_BINARY_DIR}/_deps/glfw-src/include/"
    "${PROJECT_BINARY_DIR}/_deps/imgui-docking-src/"
    "${PROJECT_BINARY_DIR}/_deps/imgui-docking-src/backends/"
    "${PROJECT_BINARY_DIR}/_deps/imgui-docking-src/misc/cpp"
    "${PROJECT_BINARY_DIR}/_deps/nfd-src/src/include"
)

# link libraries to the executable
target_link_libraries(${PROJECT_NAME} PRIVATE
    "glfw;libglew_static;glm;nfd"
)

# add definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC
    NFD_THROWS_EXCEPTIONS # for the nfd-extended library
)