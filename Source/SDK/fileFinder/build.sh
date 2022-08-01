#!/bin/bash

# Assuming that you called build.sh from its own folder, you should already be in the project folder.
PROJECT_FOLDER=.
# Placing your executable in the project folder allow using the same relative paths in the final release.
TARGET_FILE=./application
# The root folder is where DFPSR, SDK and tools are located.
ROOT_PATH=../..
# Select where to place temporary files.
TEMP_DIR=${ROOT_PATH}/../../temporary
# Select a window manager
WINDOW_MANAGER=NONE
# Select safe debug mode or fast release mode
#MODE=-DDEBUG #Debug mode
MODE=-DNDEBUG #Release mode
COMPILER_FLAGS="${MODE} -std=c++14 -O2"
# Select external libraries
LINKER_FLAGS=""

# Give execution permission
chmod +x ${ROOT_PATH}/tools/buildAndRun.sh;
# Compile everything
${ROOT_PATH}/tools/build.sh "${PROJECT_FOLDER}" "${TARGET_FILE}" "${ROOT_PATH}" "${TEMP_DIR}" "${WINDOW_MANAGER}" "${COMPILER_FLAGS}" "${LINKER_FLAGS}";
# Run the application
${TARGET_FILE} ./পরীক্ষা;
