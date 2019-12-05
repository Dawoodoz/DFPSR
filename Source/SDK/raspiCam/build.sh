#!/bin/bash

# Assuming that you called build.sh from its own folder, you should already be in the project folder.
PROJECT_FOLDER=.
# Placing your executable in the project folder allow using the same relative paths in the final release.
TARGET_FILE=./camera
# The root folder is where DFPSR, SDK and tools are located.
ROOT_PATH=../..
# Select where to place temporary files and the generated executable
TEMP_DIR=${ROOT_PATH}/../../temporary
# Select a window manager
WINDOW_MANAGER=X11
# Select safe debug mode or fast release mode
#MODE=-DDEBUG #Debug mode
MODE=-DNDEBUG #Release mode
# Select the version of C++
CPP_VERSION=-std=c++14
# Select optimization level
O_LEVEL=-O2
# Select external libraries
LINKER_FLAGS="-lraspicam"

# Give execution permission
chmod +x ${ROOT_PATH}/tools/buildAndRun.sh;
# Compile everything
${ROOT_PATH}/tools/buildAndRun.sh ${PROJECT_FOLDER} ${TARGET_FILE} ${ROOT_PATH} ${TEMP_DIR} ${WINDOW_MANAGER} ${MODE} ${CPP_VERSION} ${O_LEVEL} ${LINKER_FLAGS};

