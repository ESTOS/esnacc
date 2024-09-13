#!/bin/bash

if ! command -v cmake &> /dev/null
then
    echo "Could not find cmake in PATH"
    echo "Ensure that cmake is in PATH before calling this script"
    sleep 5
    exit 1
fi

# Minimum requirement for CMake version
REQUIRED_CMAKE_VERSION="3.20"

# Function to compare version numbers
parse_cmake_version() {
  [ "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1" ]
}

# Retrieve current CMake version
CURRENT_CMAKE_VERSION=$(cmake --version | head -n 1 | awk '{print $3}')

# Version check
if parse_cmake_version $CURRENT_CMAKE_VERSION $REQUIRED_CMAKE_VERSION; then
  echo "CMake version $CURRENT_CMAKE_VERSION meets the minimum requirement of $REQUIRED_CMAKE_VERSION."
else
  echo "CMake version $CURRENT_CMAKE_VERSION does not meet the minimum requirement of $REQUIRED_CMAKE_VERSION."
  exit 1
fi

# Create build directories
mkdir -p build/debug
mkdir -p build/release

# Debug build
cd build/debug
cmake -DCMAKE_BUILD_TYPE=debug ../..
cmake --build . --config debug

# Release build
cd ../release
cmake -DCMAKE_BUILD_TYPE=release ../..
cmake --build . --config release

# back to the roots
cd ../..

# Build completed.
