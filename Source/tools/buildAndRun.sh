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

TEMP_SUB="${MODE}_${CPP_VERSION}_${O_LEVEL}"
TEMP_SUB=$(echo $TEMP_SUB | tr "+" "p")
TEMP_SUB=$(echo $TEMP_SUB | tr -d " =-")
TEMP_DIR=${TEMP_ROOT}/${TEMP_SUB}
echo "Building version ${TEMP_SUB}"

# Create a temporary folder
mkdir -p ${TEMP_DIR}
# Remove objects, but keep libraries
rm -f ${TEMP_DIR}/*.o

# Allow calling the build script
chmod +x ${ROOT_PATH}/tools/buildLibrary.sh

echo "Compiling renderer framework."
${ROOT_PATH}/tools/buildLibrary.sh g++ ${ROOT_PATH}/DFPSR ${TEMP_DIR} "dfpsr" ${CPP_VERSION} ${O_LEVEL} ${MODE} LAZY
if [ $? -ne 0 ]
then
	exit 1
fi

# Abort if the project folder is replaced with the NONE keyword
if [ ${PROJECT_FOLDER} = "NONE" ]
then
	exit 0
fi

echo "Compiling application."
${ROOT_PATH}/tools/buildLibrary.sh g++ ${PROJECT_FOLDER} ${TEMP_DIR} "application" ${CPP_VERSION} ${O_LEVEL} ${MODE} CLEAN
if [ $? -ne 0 ]
then
	exit 1
fi

# Select the base libraries needed by the framework itself
BASELIBS="-lm -pthread"

# Select window manager to compile and libraries to link
if [ ${WINDOW_MANAGER} = "NONE" ]
then
	# Embedded/terminal mode
	WINDOW_SOURCE=${ROOT_PATH}/windowManagers/NoWindow.cpp
	LIBS="${BASELIBS} ${LINKER_FLAGS}"
else
	# Desktop GUI mode
	WINDOW_SOURCE=${ROOT_PATH}/windowManagers/${WINDOW_MANAGER}Window.cpp
	LIBS="${BASELIBS} ${LINKER_FLAGS} -l${WINDOW_MANAGER}"
fi

echo "Compiling window manager (${WINDOW_SOURCE})"
g++ ${CPP_VERSION} ${O_LEVEL} ${MODE} -Wall -c ${WINDOW_SOURCE} -o ${TEMP_DIR}/NativeWindow.o
if [ $? -ne 0 ]
then
	exit 1
fi
echo "Linking application with libraries (${LIBS})"
# Main must exist in the first library when linking
g++ ${TEMP_DIR}/application.a ${TEMP_DIR}/dfpsr.a ${TEMP_DIR}/NativeWindow.o ${LIBS} -o ${TARGET_FILE}
if [ $? -ne 0 ]
then
	exit 1
fi

echo "Starting application at ${TARGET_FILE}"
${TARGET_FILE}
