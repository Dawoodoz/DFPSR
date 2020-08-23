#!/bin/bash

# Load arguments into named variables in order
PROJECT_FOLDER=$1 # Where your code is
TARGET_FILE=$2 # Your executable to build
ROOT_PATH=$3 # The parent folder of DFPSR, SDK and tools
TEMP_ROOT=$4 # Where your temporary objects should be
WINDOW_MANAGER=$5 # Which library to use for creating a window
MODE=$6 # Use -DDEBUG for debug mode or -DNDEBUG for release mode
CPP_VERSION=$7 # Default is -std=c++14
O_LEVEL=$8 # Default is -O2
LINKER_FLAGS=$9 # Additional linker flags for libraries and such

# Allow calling the build script
chmod +x ${ROOT_PATH}/tools/build.sh

# Compile and link
${ROOT_PATH}/tools/build.sh $1 $2 $3 $4 $5 $6 $7 $8 $9
if [ $? -ne 0 ]
then
	exit 1
fi

echo "Starting application at ${TARGET_FILE}"
${TARGET_FILE}
