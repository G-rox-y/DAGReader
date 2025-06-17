cmake_minimum_required(VERSION 3.24)

# project init
project(DAGReader VERSION 1.0)

# default build type is debug
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified. Defaulting to Debug.")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
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

# i want dependencies to be built as release, so i will just change the build type not to interfere
set(root_build_type "${CMAKE_BUILD_TYPE}")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)

# some also throw so many compiler warnings, they are not related to this project so best to silence them
set(root_cxx_flags "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w") 

include(FetchContent)

# add glfw
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
set(ONLY_LIBS ON CACHE INTERNAL "" FORCE)
FetchContent_MakeAvailable(glfw)

# add glew-cmake
FetchContent_Declare(glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake
    GIT_TAG 9758219375d923a008feef93c0955931d801740f # master branch Jun 26 2023
)
# if no debugging is happening i can turn off the building of utility programs (glewinfo and visualinfo)
# this is because glewinfo is used to list extensions and visualinfo to list visual/pixel formats
if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BUILD_UTILS OFF CACHE BOOL "")
endif()
set(glew-cmake_BUILD_SHARED OFF CACHE INTERNAL "" FORCE)
FetchContent_MakeAvailable(glew)

# add glm
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# add imgui
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.91.9
)
FetchContent_MakeAvailable(imgui)

# add nfd-extended
# TODO: test for dependencies and their availlability (same goes for other packages)
FetchContent_Declare(nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
    GIT_TAG v1.2.1
)
# Disable installation of NFD library to the system
set(NFD_INSTALL OFF CACHE BOOL "")
FetchContent_MakeAvailable(nfd)

# add speedlog (header only)
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(spdlog)

# add inipp
FetchContent_Declare(inipp
    GIT_REPOSITORY https://github.com/mcmtroffaes/inipp
    GIT_TAG 46248e4e93a2e63f9a1d0d8d9ad40bd6b3725df5 # develop branch 12 Oct 2021
)
FetchContent_MakeAvailable(inipp)

# and return stuff to normal
set(CMAKE_BUILD_TYPE "${root_build_type}" CACHE STRING "Build type" FORCE)
set(CMAKE_CXX_FLAGS "${root_cxx_flags}")

# -------------------- END OF DEPENDENCIES

# create executable
add_executable(${PROJECT_NAME})

# add code
add_subdirectory("src")

# add shaders
add_custom_command(
    OUTPUT
        ${CMAKE_BINARY_DIR}/shaderEmbed.cpp
    COMMAND ${CMAKE_COMMAND} 
        -DINPUT_VERT=${CMAKE_SOURCE_DIR}/src/shaders/vertex.glsl
        -DINPUT_TCS=${CMAKE_SOURCE_DIR}/src/shaders/tcs.glsl
        -DINPUT_TES=${CMAKE_SOURCE_DIR}/src/shaders/tes.glsl
        -DINPUT_GEOM=${CMAKE_SOURCE_DIR}/src/shaders/geometry.glsl
        -DINPUT_FRAG=${CMAKE_SOURCE_DIR}/src/shaders/fragment.glsl
        -DOUTPUT_FILE=${CMAKE_BINARY_DIR}/shaderEmbed.cpp
        -P ${CMAKE_SOURCE_DIR}/src/shaders/shaderEmbed.cmake
    DEPENDS 
        ${CMAKE_SOURCE_DIR}/src/shaders/vertex.glsl
        ${CMAKE_SOURCE_DIR}/src/shaders/tcs.glsl
        ${CMAKE_SOURCE_DIR}/src/shaders/tes.glsl
        ${CMAKE_SOURCE_DIR}/src/shaders/geometry.glsl
        ${CMAKE_SOURCE_DIR}/src/shaders/fragment.glsl
)

# add imguis code since they dont have their own (useable) cmake
target_sources(${PROJECT_NAME} PUBLIC
    "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp" # primarily for std::string
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"  # imgui demo (optional)
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
    "${PROJECT_BINARY_DIR}/shaderEmbed.cpp" # this is where the shaders get added to
)

# include dependencies
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    "${glm_SOURCE_DIR}/"
    "${glew_SOURCE_DIR}/include/"
    "${glfw_SOURCE_DIR}/include/"
    "${imgui_SOURCE_DIR}/"
    "${imgui_SOURCE_DIR}/backends/"
    "${imgui_SOURCE_DIR}/misc/cpp"
    "${nfd_SOURCE_DIR}/src/include"
    "${spdlog_SOURCE_DIR}/include"
    "${inipp_SOURCE_DIR}/"
)

# link libraries to the executable
target_link_libraries(${PROJECT_NAME} PRIVATE
    "glfw;libglew_static;nfd;inipp"
)

# add definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC
    NFD_THROWS_EXCEPTIONS # for the nfd-extended library
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG_LOGGING)
endif()