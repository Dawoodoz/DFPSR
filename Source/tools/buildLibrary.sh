#!/bin/bash

# Compile all cpp files in a folder and all of its sub-folders into a static library using the GNU c++ compiler

# The global command for running the compiler (tested with g++ and clang++)
COMPILER=$1
# The root of each folder containing source files
SOURCE_FOLDERS=$2
# The target folder where the library will be created
TARGET=$3
# The name of your library without any path nor extension
LIBRARY_NAME=$4
OBJECT_POSTFIX="_${LIBRARY_NAME}_TEMP.o"
# Compiler flags
COMPILER_FLAGS=$5
# Use CLEAN to recompile everything
# Use LAZY to only recompile if the source folder itself has changed
#   If the library depends on anything outside of its folder that changes, lazy compilation will fail
#   If you change modes a lot and compiler versions a lot, multiple temporary folders may be useful
BUILD_METHOD=$6

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
	NEW_SUM="$(tar cf - ${SOURCE_FOLDERS} | md5sum)"
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
if [ ! -f ${LIBRARY_FILENAME} ]
then
	# Argument: $1 as the folder to compile recursively
	compileFolder() {
		if [ ! -d "$1" ]; then
			echo "Failed to compile files in $1 because the folder does not exist!"
			exit 1
		fi
		# Compile files in the folder
		for file in "$1"/*.cpp
		do
			[ -e $file ] || continue
			# Get name without path
			name=${file##*/}
			# Get name without extension nor path
			base=${name%.cpp}
			echo "  C++ ${file}"
			${COMPILER} ${COMPILER_FLAGS} -Wall -c ${file} -o ${TARGET}/${base}${OBJECT_POSTFIX}
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

	# Split the space separated folders into an array
	OLD_IFS="${IFS}"
	IFS=" "
	read -ra FOLDER_ARRAY <<< "$SOURCE_FOLDERS"
	IFS="${OLD_IFS}" # IFS must be brought back to avoid crashing the compiler
	#Compile each of the project folders (Only one of them may contain main, so the rest must be libraries)
	for FOLDER in "${FOLDER_ARRAY[@]}"; do
		echo "Compiling ${FOLDER} using ${COMPILER_FLAGS}."
		compileFolder "${FOLDER}"
		if [ $? -ne 0 ]
		then
			exit 1
		fi
	done

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
