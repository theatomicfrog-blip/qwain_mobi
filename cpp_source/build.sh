#!/bin/bash

# Fix broken packages
# sudo apt-get update
# sudo apt-get --fix-broken install -y

# For this Qt Widgets app, you do NOT need qtdeclarative5-dev.
# You only need qtbase5-dev
# sudo apt-get install -y build-essential cmake qt6-base-dev libqt6svg6-dev ninja-build

# Fix for "build.ninja still dirty" - update file timestamps to current system time
find .. -type f -exec touch {} + 2>/dev/null || true

# Build the project
cd ..
rm -rf build
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make
