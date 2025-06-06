#!/bin/bash

# Assuming that you called build.sh from its own folder, you should already be in the project folder.
PROJECT_FOLDERS=". ../SoundEngine"
# Placing your executable in the project folder allow using the same relative paths in the final release.
TARGET_FILE=./Music
# The root folder is where DFPSR, SDK and tools are located.
ROOT_PATH=../..
# Select where to place temporary files.
TEMP_DIR=${ROOT_PATH}/../../temporary
# Select a window manager
MANAGERS="X11&ALSA"
# Select safe debug mode or fast release mode
#MODE=-DDEBUG #Debug mode
MODE=-DNDEBUG #Release mode
COMPILER_FLAGS="${MODE} -std=c++14 -O2"
# Select external libraries
LINKER_FLAGS=""

# Give execution permission
chmod +x ${ROOT_PATH}/tools/buildScripts/buildAndRun.sh;
# Compile everything
${ROOT_PATH}/tools/buildScripts/buildAndRun.sh "${PROJECT_FOLDERS}" "${TARGET_FILE}" "${ROOT_PATH}" "${TEMP_DIR}" "${MANAGERS}" "${COMPILER_FLAGS}" "${LINKER_FLAGS}";
