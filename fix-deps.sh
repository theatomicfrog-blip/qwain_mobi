#!/bin/bash
find build/CMakeFiles/Qwain.dir -type f -name "*.o" | while read f; do
    touch "${f}.d"
done
