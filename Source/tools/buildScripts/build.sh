#!/bin/bash

# Load arguments into named variables in order
PROJECT_FOLDERS=$1 # Where your code is as a space separated list of folders in a quote
TARGET_FILE=$2 # Your executable to build
ROOT_PATH=$3 # The parent folder of DFPSR, SDK and tools
TEMP_ROOT=$4 # Where your temporary objects should be
MANAGERS=$5 # Which libraries to use for creating windows and playing sounds
COMPILER_FLAGS=$6 # -DDEBUG/-DNDEBUG -std=c++14/-std=c++17 -O2/-O3
LINKER_FLAGS=$7 # Additional linker flags for libraries and such

# Replace space with underscore
TEMP_SUB="${COMPILER_FLAGS// /_}"
# Replace + with p
TEMP_SUB=$(echo $TEMP_SUB | tr "+" "p")
# Remove = and -
TEMP_SUB=$(echo $TEMP_SUB | tr -d "=-")
TEMP_DIR=${TEMP_ROOT}/${TEMP_SUB}
echo "Building version ${TEMP_SUB}"

# Allow calling other scripts
chmod +x ${ROOT_PATH}/tools/buildScripts/clean.sh
chmod +x ${ROOT_PATH}/tools/buildScripts/buildLibrary.sh

# Make a clean folder
${ROOT_PATH}/tools/buildScripts/clean.sh ${TEMP_DIR}

echo "Compiling renderer framework."
${ROOT_PATH}/tools/buildScripts/buildLibrary.sh g++ ${ROOT_PATH}/DFPSR ${TEMP_DIR} "dfpsr" "${COMPILER_FLAGS}" LAZY
if [ $? -ne 0 ]
then
	exit 1
fi

# Abort if the project folder is replaced with the NONE keyword
if [ "${PROJECT_FOLDERS}" = "NONE" ]
then
	exit 0
fi

echo "Compiling application."
${ROOT_PATH}/tools/buildScripts/buildLibrary.sh g++ "${PROJECT_FOLDERS}" ${TEMP_DIR} "application" "${COMPILER_FLAGS}" CLEAN
if [ $? -ne 0 ]
then
	exit 1
fi

# Select the base libraries needed by the framework itself
BASELIBS="-lm -pthread"

# Select window manager to compile and libraries to link
if [ ${MANAGERS} = "NONE" ]
then
	# Embedded/terminal mode
	WINDOW_SOURCE=${ROOT_PATH}/windowManagers/NoWindow.cpp
	SOUND_SOURCE=${ROOT_PATH}/soundManagers/NoSound.cpp
	LIBS="${BASELIBS} ${LINKER_FLAGS}"
elif [ ${MANAGERS} = "X11" ]
then
	# Desktop GUI mode without sound
	WINDOW_SOURCE=${ROOT_PATH}/windowManagers/X11Window.cpp
	SOUND_SOURCE=${ROOT_PATH}/soundManagers/NoSound.cpp
	LIBS="${BASELIBS} ${LINKER_FLAGS} -lX11"
elif [ ${MANAGERS} = "X11&ALSA" ]
then
	# Desktop GUI mode with sound
	WINDOW_SOURCE=${ROOT_PATH}/windowManagers/X11Window.cpp
	SOUND_SOURCE=${ROOT_PATH}/soundManagers/AlsaSound.cpp
	LIBS="${BASELIBS} ${LINKER_FLAGS} -lX11 -lasound"
else
	echo "Unrecognized argument ${MANAGERS} for sound and window managers!"
	exit 1
fi

echo "Compiling sound manager (${SOUND_SOURCE})"
g++ ${COMPILER_FLAGS} -Wall -c ${SOUND_SOURCE} -o ${TEMP_DIR}/NativeSound.o
if [ $? -ne 0 ]
then
	exit 1
fi
echo "Compiling window manager (${WINDOW_SOURCE})"
g++ ${COMPILER_FLAGS} -Wall -c ${WINDOW_SOURCE} -o ${TEMP_DIR}/NativeWindow.o
if [ $? -ne 0 ]
then
	exit 1
fi
echo "Linking application with libraries (${LIBS})"
# Main must exist in the first library when linking
g++ ${TEMP_DIR}/application.a ${TEMP_DIR}/NativeWindow.o ${TEMP_DIR}/NativeSound.o ${LIBS} ${TEMP_DIR}/dfpsr.a -o ${TARGET_FILE}
if [ $? -ne 0 ]
then
	exit 1
fi
