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

The following method uses MSYS2 to build the program, I am aware this is also possible to do with Visual Studio, but this method requires less external tools.

### Installing dependencies

First, you will have to install MSYS2 (https://www.msys2.org/).

After that, you should run the newly installed UCRT64 shell.

> [!WARNING]
> If MSYS2 was already installed on your system you should open the MSYS2 shell and run `pacman -Syu`, then close it and do the same in the UCRT64 shell, this is important to make sure the environment is up to date. If MSYS2 was not on your system you can safely ignore this warning

After starting the shell install dependencies
```bash
pacman -S --needed git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```


### Building the project
> [!IMPORTANT]
> Use only the UCRT64 shell for the build steps below. Don’t mix different shells (UCRT, MINGW, CLANG) within the same build directory.

1. From inside the **MSYS2 UCRT64 shell**, navigate to the folder where you want to clone the project, and clone the repo

```bash
git clone "https://github.com/G-rox-y/DAGReader" && cd ./DAGReader
```

> [!NOTE]
> If you dont know how to navigate, doing `cd /c/` will put you on the root of the C drive, same goes for `cd /d/` and the D drive, then you can cd into the correct folder

2. After that, create the build
```bash
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
```

3. And build the project
```bash
cmake --build build -j
```

4. And finally run the newly created binary
```bash
./build/DAGReader.exe
```
