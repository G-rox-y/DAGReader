# DAGReader

![DAGReader showcase video](misc/orbitVideo.mp4)

**3D Interactive Visualization for Genome Assembly Graphs (GFA)**

DAGReader is a desktop bioinformatics tool that reads [Graphical Fragment Assembly (GFA)](https://github.com/GFA-spec/GFA-spec) files and renders the underlying assembly graphs in three dimensions. It is inspired by [Bandage](https://rrwick.github.io/Bandage/) but built from the ground up as a native 3D viewer with real-time layout, selection, and exploration.

![License](https://img.shields.io/badge/license-zlib-blue)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
[![CMake](https://img.shields.io/badge/build-CMake-green)]()

## Why DAGReader?

Genome assembly graphs can be massive, tangled, and difficult to reason about in 2D alone. DAGReader uses the **GRIP (Graph dRawing with Intelligent Placement)** algorithm to compute a 3D layout that you can fly through, inspect, and query interactively. Segments and links are rendered as Bézier tubes with full camera control, selection, and search.

The code has been optimized for efficiency and works well for large graphs.

> [!NOTE]
> This project is under active development and was originally created as part of a bachelor's thesis.

## Features

- **Native 3D rendering** — OpenGL 4.3+ pipeline with tessellation and geometry shaders; edges are drawn as curved 3D tubes, not flat lines.
- **GRIP layout engine** — Multi-scale force-directed placement (Kamada–Kawai + Fruchterman–Reingold) accelerated with OpenMP.
- **GFA v1.0 support** — Parses segments, links, containments, and paths. Optional *minimal memory mode* skips large optional fields to help fit huge graphs in RAM.
- **Interactive GUI (Dear ImGui)** — Fully controllable side panel with:
  - Live layout parameter tuning (rounds, temperature, scaling)
  - Appearance controls (colors, widths, randomization)
  - Subgraph visibility & selection tables
  - Search by name (strict + fuzzy) across segments, links, paths, and containments
- **Selection & inspection** — Click to select segments/links; view metadata, sequence data, and CIGAR strings in floating HUDs.
- **Camera controls** — Free-fly, pan, rotate, zoom, and *orbit* around selections.
- **Cross-platform** — Linux, macOS, and Windows (MSYS2).

## Quick Start

### Dependencies

You only need **CMake**, **git**, and a **C++17 compiler**. All other dependencies (GLFW, GLEW, GLM, Dear ImGui, nativefiledialog-extended, spdlog, inipp) are fetched automatically via CMake `FetchContent`.

| Platform | Install command |
|----------|-----------------|
| Debian / Ubuntu | `sudo apt update && sudo apt install cmake git build-essential libomp-dev` |
| Fedora | `sudo dnf install cmake git gcc-c++ libomp-devel` |
| Arch | `sudo pacman -Syu cmake git base-devel openmp` |
| macOS | `xcode-select --install` then `brew install cmake libomp` |


### Build

```bash
git clone https://github.com/G-rox-y/DAGReader.git
cd DAGReader

# Release build (recommended)
cmake -B build -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

> [!NOTE]
> if the cmake step fails due to some missing dependencies, check out the [Full Dependency Reference](#full-dependency-reference)

### Run

```bash
./build/DAGReader        # Linux / macOS
./build/DAGReader.exe    # Windows (MSYS2)
```

## Windows Build (MSYS2)

1. Install [MSYS2](https://www.msys2.org/) and open the **UCRT64** shell.
2. Update the environment: `pacman -Syu`
3. Install toolchains:
   ```bash
   pacman -S --needed git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
   ```
4. Build:
   ```bash
   cd /c/path/to/DAGReader
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j
   ./build/DAGReader.exe
   ```


## Full Dependency Reference

If the minimal install above fails during the CMake configuration step, your system is likely missing the OS-level graphics, windowing, or desktop-integration libraries that **cannot** be fetched automatically.

### What you need and why

| Requirement | Needed by | Typical package names |
|---|---|---|
| **CMake** ≥ 3.24 | Build system | `cmake` |
| **Git** | Fetching dependencies | `git` |
| **C++17 compiler** | The project itself | `build-essential`, `gcc-c++`, `mingw-w64-ucrt-x86_64-gcc` |
| **OpenMP** *(optional)* | GRIP layout acceleration | `libomp-dev`, `libomp-devel`, `openmp` |
| **pkg-config / pkgconf** | NFD, GLEW, GLFW discovery | `pkg-config`, `pkgconf-pkg-config`, `pkgconf` |
| **OpenGL / Mesa dev headers** | GLEW + OpenGL context | `libgl1-mesa-dev`, `mesa-libGL-devel`, `mesa` |
| **X11 development libraries** | GLFW on X11 | `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev` |
| **Wayland dev libraries** *(optional)* | GLFW on Wayland | `libwayland-dev`, `wayland-protocols` |
| **GTK3 development files** | nativefiledialog-extended (file dialogs on Linux) | `libgtk-3-dev`, `gtk3-devel`, `gtk3` |

### Per-distro copy-paste commands

Use the block that matches your distribution if the Quick Start table was insufficient.

#### Debian / Ubuntu / Mint
```bash
sudo apt update && sudo apt install -y \
  cmake git build-essential libomp-dev \
  pkg-config libgl1-mesa-dev \
  libgtk-3-dev \
  libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxkbcommon-dev \
  libwayland-dev wayland-protocols
```

#### Fedora
```bash
sudo dnf install -y \
  cmake git gcc-c++ libomp-devel \
  pkgconf-pkg-config mesa-libGL-devel \
  gtk3-devel \
  libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel libxkbcommon-devel \
  wayland-devel wayland-protocols-devel
```

#### Arch / Manjaro
```bash
sudo pacman -Syu --needed \
  cmake git base-devel openmp \
  pkgconf mesa \
  gtk3 \
  libxrandr libxinerama libxcursor libxi libxkbcommon \
  wayland wayland-protocols
```


## Technical Overview

- **Renderer:** Custom OpenGL backend using programmable pipeline (vertex → tessellation control → tessellation evaluation → geometry → fragment). Edges are rendered as variable-width Bézier tube segments with normal-based lighting hints directly in the geometry shader.
- **Layout:** GRIP builds a hierarchy of filtered graphs and places vertices from coarse to fine, combining Kamada–Kawai neighborhood forces with Fruchterman–Reingold repulsion. Parameters are exposed in the UI for experimentation.
- **Threading:** Multi-threaded architecture with a dedicated window/render thread and a controller thread that handles file I/O, parsing, and layout computation. Communication is via a lock-free-ish channel with condition variables and atomics.
- **Parser:** Hand-written GFA parser with strict validation warnings. Can perform reverse-lookup by name, fuzzy matching, and on-demand sequence/CIGAR streaming.

## License & Credits

DAGReader is licensed under the **[zlib License](LICENSE)**.

Created by **Petar Dušević** with mentorship from **Krešimir Križanović**.

### Acknowledgments

The exsistance of the following open-source projects has greatly helped in the creation of DAGReader:

| Project | Purpose |
|---------|---------|
| [GLFW](https://github.com/glfw/glfw) | Windowing and input handling |
| [glew-cmake](https://github.com/Perlmint/glew-cmake) | OpenGL extension loading |
| [GLM](https://github.com/g-truc/glm) | Graphics math library |
| [Dear ImGui](https://github.com/ocornut/imgui) | Immediate-mode GUI |
| [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended) | Cross-platform native file dialogs |
| [spdlog](https://github.com/gabime/spdlog) | Fast C++ logging |
| [inipp](https://github.com/mcmtroffaes/inipp) | Simple INI file parsing |

The embedded UI font is Roboto Medium, distributed alongside Dear ImGui.
Optional multithreading support is provided by OpenMP.

## Get involved

The codebase has been written in a way that tries to minimize the amount of code while trying to keep things readable and manageable for the developer, this is done to invite contribution and should be respected in any pull requests.

If you have ideas, issues, or want to collaborate, feel free to open a ticket or a pull request.
