#!/bin/bash
mkdir -p build
cd build
cmake ..
# Compile with 4 jobs
make -j4

# Only run if compile succeeded
if [ $? -eq 0 ]; then
    ./TemplateCreator
else
    echo "Make failed to compile the project."
    exit 1
fi
