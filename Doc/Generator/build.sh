#!/bin/bash

LIBRARY_PATH=../../Source/DFPSR
DEPENDENCIES="${LIBRARY_PATH}/collection/collections.cpp ${LIBRARY_PATH}/api/fileAPI.cpp ${LIBRARY_PATH}/api/bufferAPI.cpp ${LIBRARY_PATH}/api/stringAPI.cpp ${LIBRARY_PATH}/base/SafePointer.cpp"

g++ main.cpp -o generator ${DEPENDENCIES} -std=c++14 -lm

# Execute the generation script to see the changes
chmod +x gen.sh
./gen.sh
