#!/bin/bash
# Cross-compiles Qwain with MXE and packages runtime DLLs & plugins for Windows 10 distribution

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR"

BUILD_DIR="$PROJECT_ROOT/build_win10"
DIST_DIR="$PROJECT_ROOT/dist_win10"

echo "=== Building Qwain for Windows 10 via MXE ==="

# Locate MXE installation directory
MXE_PREFIX=""
if [ -d "$HOME/mxe/usr/x86_64-w64-mingw32.shared" ]; then
    MXE_PREFIX="$HOME/mxe/usr/x86_64-w64-mingw32.shared"
elif [ -d "/opt/mxe/usr/x86_64-w64-mingw32.shared" ]; then
    MXE_PREFIX="/opt/mxe/usr/x86_64-w64-mingw32.shared"
elif [ -d "/usr/lib/mxe/usr/x86_64-w64-mingw32.shared" ]; then
    MXE_PREFIX="/usr/lib/mxe/usr/x86_64-w64-mingw32.shared"
elif [ -d "$HOME/mxe/usr/x86_64-w64-mingw32.static" ]; then
    MXE_PREFIX="$HOME/mxe/usr/x86_64-w64-mingw32.static"
fi

if [ -z "$MXE_PREFIX" ]; then
    echo "Error: MXE installation directory not found in $HOME/mxe or /opt/mxe"
    exit 1
fi

CMAKE_CMD="x86_64-w64-mingw32.shared-cmake"
if ! command -v $CMAKE_CMD &> /dev/null; then
    if [ -x "$HOME/mxe/usr/bin/x86_64-w64-mingw32.shared-cmake" ]; then
        CMAKE_CMD="$HOME/mxe/usr/bin/x86_64-w64-mingw32.shared-cmake"
    elif [ -x "$HOME/mxe/usr/bin/x86_64-w64-mingw32.static-cmake" ]; then
        CMAKE_CMD="$HOME/mxe/usr/bin/x86_64-w64-mingw32.static-cmake"
    else
        CMAKE_CMD="cmake"
    fi
fi

mkdir -p "$BUILD_DIR"
$CMAKE_CMD -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_PREFIX_PATH="$MXE_PREFIX/qt5;$MXE_PREFIX"
cmake --build "$BUILD_DIR" -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "=== Assembling Windows 10 Distribution Folder ==="
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

# Copy ONLY main Qwain.exe executable into release distribution
cp "$BUILD_DIR/Qwain.exe" "$DIST_DIR/" 2>/dev/null || cp "$BUILD_DIR/cpp_source/Qwain.exe" "$DIST_DIR/" 2>/dev/null

# For MXE shared builds, bundle runtime DLLs and required Qt plugin folders
if [ -d "$MXE_PREFIX/bin" ]; then
    echo "Bundling Qt5 and MinGW runtime DLLs..."
    cp "$MXE_PREFIX/bin/"*.dll "$DIST_DIR/" 2>/dev/null || true
    cp "$MXE_PREFIX/qt5/bin/"*.dll "$DIST_DIR/" 2>/dev/null || true

    # Platform plugin (required for Windows GUI initialization)
    mkdir -p "$DIST_DIR/platforms"
    cp "$MXE_PREFIX/qt5/plugins/platforms/qwindows.dll" "$DIST_DIR/platforms/" 2>/dev/null || true

    # SQLite database driver plugin
    mkdir -p "$DIST_DIR/sqldrivers"
    cp "$MXE_PREFIX/qt5/plugins/sqldrivers/"*.dll "$DIST_DIR/sqldrivers/" 2>/dev/null || true

    # Image format plugins
    mkdir -p "$DIST_DIR/imageformats"
    cp "$MXE_PREFIX/qt5/plugins/imageformats/"*.dll "$DIST_DIR/imageformats/" 2>/dev/null || true
fi

echo "=== Creating Standalone Qwain_Win10_x64.zip ==="
cd "$PROJECT_ROOT"
zip -r Qwain_Win10_x64.zip dist_win10/

echo "Success! Standalone Windows 10 release package created at: $PROJECT_ROOT/Qwain_Win10_x64.zip"
