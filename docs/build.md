# Building esnacc

This guide covers compiling the compiler and C++ runtime from source. For a short overview of the repository, see [ReadMe.md](../ReadMe.md).

## Prerequisites

- [CMake](https://cmake.org) 3.20 or newer, available on `PATH`

## Command line

From the repository root:

```shell
mkdir build
cd build
cmake ..
cmake --build .
```

Outputs (defaults):

| Artifact | Location |
|----------|----------|
| Compiler (`esnacc` / `esnaccd`) | `output/bin/` |
| C++ library (`esnacc_cpp_lib`) | `output/libx64/` (x64) or `output/lib/` (x86) |

### CMake variables

| Variable | Purpose |
|----------|---------|
| `CPP_LIBRARY_OUTPUT_PATH` | Output directory for the C++ library |
| `CPP_LIBRARY_OUTPUT_NAME` | Library base name (debug builds append `d`) |
| `COMPILER_OUTPUT_PATH` | Output directory for the compiler executable |
| `COMPILER_OUTPUT_NAME` | Release compiler name (default `esnacc`) |
| `COMPILER_OUTPUT_NAME_DEBUG` | Debug compiler name (default `esnaccd`) |

## Visual Studio 2022

1. Open Visual Studio and choose **Open a local folder** (not Open Project).
2. Select the repository root (contains the top-level `CMakeLists.txt`).
3. Let CMake configure (creates an `out/` directory).
4. Set the startup item to `esnacc.exe`.
5. Build with **F7** or **Ctrl+Shift+B**.

## CLion

1. Open the repository root as the project.
2. Select the toolchain (Visual Studio or MinGW).
3. Build the `compiler` run configuration.

## Visual Studio Code

Prerequisite: [Ninja](https://github.com/ninja-build/ninja/releases) 1.11.1+ on `PATH`.

1. Open `esnacc.code-workspace`.
2. Install suggested extensions (C/C++, CMake Tools, CodeLLDB, Clang-Format).
3. Configure CMake with the root `CMakeLists.txt`.
4. Select the `esnacc` build target and build.

## macOS

1. Install Xcode command-line tools and CMake (e.g. via Homebrew).
2. Follow the command-line steps above.
