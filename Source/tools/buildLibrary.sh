#!/bin/bash

# Compile all cpp files in a folder and all of its sub-folders into a static library using the GNU c++ compiler

# The global command for running the compiler (tested with g++ and clang++)
COMPILER=$1
# The root of the source files
SOURCE_FOLDER=$2
# The target folder where the library will be created
TARGET=$3
# The name of your library without any path nor extension
LIBRARY_NAME=$4
OBJECT_POSTFIX="_$4_TEMP.o"
# C++ version
CPP_VERSION=$5
# Optimization level
O_LEVEL=$6
# Debug -DDEBUG or Release -DNDEBUG
MODE=$7
# Use CLEAN to recompile everything
# Use LAZY to only recompile if the source folder itself has changed
#   If the library depends on anything outside of its folder that changes, lazy compilation will fail
#   If you change modes a lot and compiler versions a lot, multiple temporary folders may be useful
BUILD_METHOD=$8

LIBRARY_FILENAME=${TARGET}/${LIBRARY_NAME}.a
SUM_FILENAME=${TARGET}/${LIBRARY_NAME}.md5

if [ ${BUILD_METHOD} = CLEAN ]
then
	echo "Clean building ${LIBRARY_NAME}"
	# Remove the old library when clean building
	rm -f ${LIBRARY_FILENAME}
fi
if [ ${BUILD_METHOD} = LAZY ]
then
	echo "Lazy building ${LIBRARY_NAME}"
	# Cat takes a filename and returns the content
	OLD_SUM="$(cat ${SUM_FILENAME})"
	# Use tar to create an archive and apply md5sum on the archive
	NEW_SUM="$(tar cf - ${SOURCE_FOLDER} | md5sum)"
	# Remove extra characters from the result
	NEW_SUM=$(echo $NEW_SUM | tr -d " \t\n\r-")
	echo "  Old md5 checksum: ${OLD_SUM}"
	echo "  New md5 checksum: $NEW_SUM"
	# Compare new and old checksum
	#   Placed in quotes to prevent taking internal spaces as argument separators
	if [ "${NEW_SUM}" != "${OLD_SUM}" ]
	then
		echo "  Checksums didn't match. Rebuilding whole library to be safe."
		rm -f ${LIBRARY_FILENAME}
	fi
	# Clear the checksum in case of aborting compilation
	echo "Compilation not completed..." > ${SUM_FILENAME}
fi

# Check if the target library already exists
if [ ! -f ${LIBRARY_FILENAME} ]; then
	# Argument: $1 as the folder to compile recursively
	compileFolder() {
		# Compile files in the folder
		for file in "$1"/*.cpp
		do
			[ -e $file ] || continue
			# Get name without path
			name=${file##*/}
			# Get name without extension nor path
			base=${name%.cpp}
			echo "  C++ ${file}"
			${COMPILER} ${CPP_VERSION} ${O_LEVEL} ${MODE} -Wall -c ${file} -o ${TARGET}/${base}${OBJECT_POSTFIX}
			if [ $? -ne 0 ]
			then
				echo "Failed to compile ${file}!"
				exit 1
			fi
		done
		# Recursively compile other folders
		for folder in "$1"/*
		do
			if [ -d "$folder" ]
			then
				compileFolder "$folder"
			fi
		done
	}
	# Compiling temporary objects
	echo "Compiling cpp files into object files in $TARGET using $MODE mode."
	compileFolder ${SOURCE_FOLDER}
	# Assembling static library
	echo "Assembling object files into ${LIBRARY_NAME}.a."
	ar rcs ${LIBRARY_FILENAME} ${TARGET}/*${OBJECT_POSTFIX}
	# Cleaning up temporary objects
	echo "Cleaning up temporary ${LIBRARY_NAME} object files."
	rm -f ${TARGET}/*${OBJECT_POSTFIX}
fi

if [ ${BUILD_METHOD} = LAZY ]
then
	# Save new checksum to file when done compiling
	echo ${NEW_SUM} > ${SUM_FILENAME}
fi
