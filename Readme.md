# DAGReader

## Building the project on Linux and macOS

This project uses [CMake](https://cmake.org/) as its buildsystem with dependencies fetched automatically via [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)
You just need CMake, git and a C++ compiler that supports c++17 or higher.

### Installing Dependencies
#### Debian based systems (Ubuntu, Mint ...)
```bash
sudo apt update && sudo apt install cmake git build-essential
```
#### Fedora based systems
```bash
sudo dnf install cmake git gcc-c++
```
#### Arch based systems
```bash
sudo pacman -Syu cmake git base-devel
```
#### macOS
1. install Xcode Command Line Tools (includes compiler + git)
```bash
xcode-select --install
```
2. install cmake via homebrew (it will also work if you install it manually)
```bash
brew install cmake
```
---
### Building instructions
1. clone the repository
```bash
git clone "https://github.com/G-rox-y/DAGReader" && cd ./DAGReader
```
2. create the build (if you want a debug version, change the -D flag to `-DCMAKE_BUILD_TYPE=Debug`)
```bash
cmake -B ./build -S . -DCMAKE_BUILD_TYPE=Release
```
3. build the project
```bash
cmake --build build
```
4. And you can now run the program thats inside the `build` folder with 
```bash
./build/DAGReader
```

## Building the project on windows
This is possible via standard Windows CMake building methods, but i havent tested it yet so thats coming soon.