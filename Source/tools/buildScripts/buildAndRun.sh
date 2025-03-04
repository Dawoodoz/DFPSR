#!/bin/bash

# Load arguments into named variables in order
PROJECT_FOLDERS=$1 # Where your code is as a space separated list of folders in a quote
TARGET_FILE=$2 # Your executable to build
ROOT_PATH=$3 # The parent folder of DFPSR, SDK and tools
TEMP_ROOT=$4 # Where your temporary objects should be
WINDOW_MANAGER=$5 # Which library to use for creating a window
COMPILER_FLAGS=$6 # -DDEBUG/-DNDEBUG -std=c++14/-std=c++17 -O2/-O3
LINKER_FLAGS=$7 # Additional linker flags for libraries and such

# Allow calling the build script
chmod +x ${ROOT_PATH}/tools/buildScripts/build.sh

# Compile and link
${ROOT_PATH}/tools/buildScripts/build.sh "$1" "$2" "$3" "$4" "$5" "$6" "$7"
if [ $? -ne 0 ]
then
	exit 1
fi

echo "Starting application at ${TARGET_FILE}"
${TARGET_FILE}
