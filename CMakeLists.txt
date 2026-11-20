cmake_minimum_required(VERSION 3.24)

# project init
project(DAGReader VERSION 1.0.5)

# default build type is debug
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified. Defaulting to Debug.")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# set important vars
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -finput-charset=UTF-8")
if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "DebugRelease")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Og -fno-omit-frame-pointer")
endif()

# -------------------- DEPENDENCIES
set(root_build_type "${CMAKE_BUILD_TYPE}")
set(root_cxx_flags "${CMAKE_CXX_FLAGS}")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
string(APPEND CMAKE_CXX_FLAGS " -w")

include(FetchContent)

macro(fetch_dep name repo tag)
    FetchContent_Declare(${name}
        GIT_REPOSITORY ${repo}
        GIT_TAG ${tag}
        GIT_SHALLOW TRUE
    )
endmacro()

set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
set(ONLY_LIBS ON CACHE INTERNAL "" FORCE)
set(glew-cmake_BUILD_SHARED OFF CACHE INTERNAL "" FORCE)
set(NFD_INSTALL OFF CACHE BOOL "" FORCE)

if(NOT root_build_type STREQUAL "Debug")
    set(BUILD_UTILS OFF CACHE BOOL "" FORCE)
endif()

fetch_dep(glfw https://github.com/glfw/glfw 3.3.8)
fetch_dep(glew https://github.com/Perlmint/glew-cmake a88b7d7f009abc89c6e25299e6f9a13ca134f7c0)
fetch_dep(glm https://github.com/g-truc/glm 1.0.3)
fetch_dep(imgui https://github.com/ocornut/imgui v1.92.8)
fetch_dep(nfd https://github.com/btzy/nativefiledialog-extended v1.3.0)
fetch_dep(spdlog https://github.com/gabime/spdlog v1.15.2)
fetch_dep(inipp https://github.com/mcmtroffaes/inipp 3f224f1eed7a67d5d7e5fc8cab72de02a056b966)
fetch_dep(rapidcsv https://github.com/d99kris/rapidcsv v8.83)

FetchContent_MakeAvailable(glfw glew glm imgui nfd spdlog inipp rapidcsv)

set(CMAKE_BUILD_TYPE "${root_build_type}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${root_cxx_flags}")

find_package(OpenMP)
if(NOT OpenMP_CXX_FOUND)
    message(WARNING "OpenMP not found. Building without CPU multithreading support.")
endif()

# -------------------- END OF DEPENDENCIES

# create executable
add_executable(${PROJECT_NAME})

# ----- take care of the source code
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
target_sources(${PROJECT_NAME} PUBLIC ${SRC_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src/pch.hpp")

target_include_directories(${PROJECT_NAME} PUBLIC
    "${PROJECT_BINARY_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/filetypes"
    "${CMAKE_SOURCE_DIR}/src/graphics"
    "${CMAKE_SOURCE_DIR}/src/layout"
)

set(DAGReader_VERSION_STRING "${DAGReader_VERSION_MAJOR}.${DAGReader_VERSION_MINOR}")
if(DEFINED DAGReader_VERSION_PATCH)
  string(APPEND DAGReader_VERSION_STRING ".${DAGReader_VERSION_PATCH}")
endif()
configure_file("${CMAKE_SOURCE_DIR}/src/cmakevars.hpp.in" "${PROJECT_BINARY_DIR}/src/cmakevars.hpp")
# ------ 

# add shaders
add_custom_command(
    OUTPUT
        ${CMAKE_BINARY_DIR}/src/shaderEmbed.cpp
    COMMAND ${CMAKE_COMMAND} 
        -DINPUT_VERT=${CMAKE_SOURCE_DIR}/src/graphics/shaders/vertex.glsl
        -DINPUT_FRAG=${CMAKE_SOURCE_DIR}/src/graphics/shaders/fragment.glsl
        -DINPUT_BEZIER=${CMAKE_SOURCE_DIR}/src/graphics/shaders/bezier.glsl
        -DOUTPUT_FILE=${CMAKE_BINARY_DIR}/src/shaderEmbed.cpp
        -P ${CMAKE_SOURCE_DIR}/src/graphics/shaders/shaderEmbed.cmake
    DEPENDS 
        ${CMAKE_SOURCE_DIR}/src/graphics/shaders/vertex.glsl
        ${CMAKE_SOURCE_DIR}/src/graphics/shaders/fragment.glsl
        ${CMAKE_SOURCE_DIR}/src/graphics/shaders/bezier.glsl
)

# Embed the license into the code
file(READ "${CMAKE_SOURCE_DIR}/LICENSE" licenseText)
foreach(var licenseText)
    string(REPLACE "\"" "\\\"" ${var} "${${var}}") # escape quotation marks
    string(REPLACE "\n" "\\n\"\n\"" ${var} "${${var}}") # convert newline characters
endforeach()
file(WRITE "${CMAKE_BINARY_DIR}/src/licenseText.cpp" "const char* licenseText = \"${licenseText}\";\n\n")

# add imguis code since they dont have their own (useable) cmake
target_sources(${PROJECT_NAME} PRIVATE
    "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp" # primarily for std::string
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
    "${PROJECT_BINARY_DIR}/src/shaderEmbed.cpp" # this is where the shaders get added to
    "${PROJECT_BINARY_DIR}/src/licenseText.cpp" # this is where the license gets added to
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
    "${rapidcsv_SOURCE_DIR}/src"
)

# link libraries to the executable
target_link_libraries(${PROJECT_NAME} PRIVATE
    "glfw;libglew_static;nfd;inipp"
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_OPENMP)
endif()

# add definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC
    NFD_THROWS_EXCEPTIONS # for the nfd-extended library
    GLM_ENABLE_EXPERIMENTAL
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "DebugRelease")
    target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG_LOGGING)
    target_sources(${PROJECT_NAME} PRIVATE "${imgui_SOURCE_DIR}/imgui_demo.cpp")
endif()

# --- Add the font to the binary ---

add_executable(bin2c ${imgui_SOURCE_DIR}/misc/fonts/binary_to_compressed_c.cpp)

set(FONT_TTF ${imgui_SOURCE_DIR}/misc/fonts/Roboto-Medium.ttf)
set(FONT_HDR ${PROJECT_BINARY_DIR}/src/Roboto_Medium.hpp)

add_custom_command(
  OUTPUT  ${FONT_HDR}
  COMMAND bin2c ${FONT_TTF} Roboto_Medium > ${FONT_HDR}
  DEPENDS bin2c ${FONT_TTF}
  COMMENT "Embedding Roboto-Medium.ttf into Roboto_Medium.hpp"
)
add_custom_target(embed_fonts DEPENDS ${FONT_HDR})

add_dependencies(${PROJECT_NAME} embed_fonts)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR})