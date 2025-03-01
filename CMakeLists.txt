cmake_minimum_required(VERSION 3.24)

# project init
project(DAGReader VERSION 0.1)

# set important vars
set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set(CMAKE_BUILD_FLAGS "-O3")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# create executable
add_executable(${PROJECT_NAME})

# add code
add_subdirectory("src")

