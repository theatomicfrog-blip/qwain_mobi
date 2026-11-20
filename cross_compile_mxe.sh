#!/bin/bash
# Helper script to cross-compile Qwain for Windows using MXE (MinGW-w64)

# Locate project root directory containing CMakeLists.txt
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR"

if [ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
    echo "Error: Cannot find CMakeLists.txt in $PROJECT_ROOT"
    exit 1
fi

BUILD_DIR="$PROJECT_ROOT/build_static"
mkdir -p "$BUILD_DIR"

echo "=== Cross-compiling Qwain for Windows via MXE ==="
echo "Project Root: $PROJECT_ROOT"
echo "Build Dir:    $BUILD_DIR"

# Locate MXE cmake wrapper toolchain launcher
CMAKE_CMD="x86_64-w64-mingw32.shared-cmake"
if ! command -v $CMAKE_CMD &> /dev/null; then
    if [ -x "$HOME/mxe/usr/bin/x86_64-w64-mingw32.shared-cmake" ]; then
        CMAKE_CMD="$HOME/mxe/usr/bin/x86_64-w64-mingw32.shared-cmake"
    elif [ -x "/opt/mxe/usr/bin/x86_64-w64-mingw32.shared-cmake" ]; then
        CMAKE_CMD="/opt/mxe/usr/bin/x86_64-w64-mingw32.shared-cmake"
    else
        CMAKE_CMD="cmake"
    fi
fi

echo "Using CMake launcher: $CMAKE_CMD"

# Auto-detect MXE Qt prefix paths
PREFIX_ARGS=""
if [ -d "$HOME/mxe/usr/x86_64-w64-mingw32.shared/qt5" ]; then
    PREFIX_ARGS="-DCMAKE_PREFIX_PATH=$HOME/mxe/usr/x86_64-w64-mingw32.shared/qt5;$HOME/mxe/usr/x86_64-w64-mingw32.shared"
elif [ -d "$HOME/mxe/usr/x86_64-w64-mingw32.shared/qt6" ]; then
    PREFIX_ARGS="-DCMAKE_PREFIX_PATH=$HOME/mxe/usr/x86_64-w64-mingw32.shared/qt6;$HOME/mxe/usr/x86_64-w64-mingw32.shared"
fi

# Run CMake explicitly providing Source (-S) and Build (-B) directories
$CMAKE_CMD -S "$PROJECT_ROOT" -B "$BUILD_DIR" $PREFIX_ARGS
if [ $? -eq 0 ]; then
    echo "=== Building Windows Executables (Qwain.exe) ==="
    cmake --build "$BUILD_DIR"
fi
