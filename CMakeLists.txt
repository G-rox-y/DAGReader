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

    # if no debugging is happening i can turn off the building of utility programs (glewinfo and visualinfo)
    # this is because glewinfo is used to list extensions and visualinfo to list visual/pixel formats
    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(BUILD_UTILS OFF CACHE BOOL "")
    endif()
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
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.91.9
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
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

    # Disable installation of NFD library to the system
    set(NFD_INSTALL OFF CACHE BOOL "")

    add_subdirectory(${nfd_SOURCE_DIR} ${nfd_BINARY_DIR})
endif()

# add speedlog (header only)
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(spdlog)

# add ogdf
FetchContent_Declare(ogdf
    GIT_REPOSITORY https://github.com/ogdf/ogdf
    GIT_TAG elderberry-202309
)
FetchContent_MakeAvailable(ogdf)

# and return stuff to normal
set(CMAKE_BUILD_TYPE "${root_build_type}" CACHE STRING "Build type" FORCE)
set(CMAKE_CXX_FLAGS "${root_cxx_flags}")

# -------------------- END OF DEPENDENCIES

# create executable
add_executable(${PROJECT_NAME})

# add code
add_subdirectory("src")

# add imguis code since they dont have their own (useable) cmake
target_sources(${PROJECT_NAME} PUBLIC
    "${PROJECT_BINARY_DIR}/_deps/imgui-src/misc/cpp/imgui_stdlib.cpp" # primarily for std::string
    "${PROJECT_BINARY_DIR}/_deps/imgui-src/imgui.cpp"
    "${PROJECT_BINARY_DIR}/_deps/imgui-src/imgui_draw.cpp"
    "${PROJECT_BINARY_DIR}/_deps/imgui-src/imgui_demo.cpp"  # delete this one before moving to the main branch
    "${PROJECT_BINARY_DIR}/_deps/imgui-src/imgui_tables.cpp"
    "${PROJECT_BINARY_DIR}/_deps/imgui-src/imgui_widgets.cpp"
    "${PROJECT_BINARY_DIR}/_deps/imgui-src/backends/imgui_impl_glfw.cpp"
    "${PROJECT_BINARY_DIR}/_deps/imgui-src/backends/imgui_impl_opengl3.cpp"
)

# include dependencies
target_include_directories(${PROJECT_NAME} PUBLIC
    "${PROJECT_BINARY_DIR}/_deps/glm-src/"
    "${PROJECT_BINARY_DIR}/_deps/glew-src/include/"
    "${PROJECT_BINARY_DIR}/_deps/glfw-src/include/"
    "${PROJECT_BINARY_DIR}/_deps/imgui-src/"
    "${PROJECT_BINARY_DIR}/_deps/imgui-src/backends/"
    "${PROJECT_BINARY_DIR}/_deps/imgui-src/misc/cpp"
    "${PROJECT_BINARY_DIR}/_deps/nfd-src/src/include"
    "${PROJECT_BINARY_DIR}/_deps/spdlog-src/include"
    "${PROJECT_BINARY_DIR}/_deps/ogdf-src/include"
)

# link libraries to the executable
target_link_libraries(${PROJECT_NAME} PRIVATE
    "glfw;libglew_static;nfd;OGDF"
)

# remove this warning for ogdf since it uses a lot of non standard pragmas
# so if you dont use clang it will fill up the terminal with a bunch of warnings
target_compile_options(OGDF PRIVATE -Wno-unknown-pragmas)

# add definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC
    NFD_THROWS_EXCEPTIONS # for the nfd-extended library
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG_LOGGING)
endif()