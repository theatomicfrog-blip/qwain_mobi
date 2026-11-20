#!/bin/bash
mkdir -p build
cd build
cmake -G "Unix Makefiles" ../cpp_source
make -j4
