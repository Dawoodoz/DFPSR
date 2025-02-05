#!/bin/bash

# Local build settings that should be configured before building for the first time.

# Make sure to erase all objects in the temporary folder before changing compiler.

# Select build method. (Not generating a script requires having the full path of the compiler.)
#GENERATE_SCRIPT="Yes"
GENERATE_SCRIPT="No"

# Change TEMPORARY_FOLDER if you do not want to recompile everything after each reboot.
TEMPORARY_FOLDER="/tmp"
COMPILER_NAME="g++"





# Find the compiler.
CPP_COMPILER_PATH=$(which "${COMPILER_NAME}")
if [ -n "$CPP_COMPILER_PATH" ]; then
	echo "Found ${COMPILER_NAME} at ${CPP_COMPILER_PATH}."
else
	echo "Could not find ${COMPILER_NAME}."
	exit 1
fi

# Get the script's folder.
BUILDER_FOLDER=$(dirname "$0")
echo "BUILDER_FOLDER = ${BUILDER_FOLDER}"

# Ask for permission to access the temporary folder.
if ! ( [ -d "${TEMPORARY_FOLDER}" ] && [ -r "${TEMPORARY_FOLDER}" ] && [ -w "${TEMPORARY_FOLDER}" ] && [ -x "${TEMPORARY_FOLDER}" ] ); then
	echo "Can not access the ${TEMPORARY_FOLDER} folder."
	TEMPORARY_FOLDER="${BUILDER_FOLDER}/temporary"
	mkdir "${TEMPORARY_FOLDER}"
	if ! ( [ -d "${TEMPORARY_FOLDER}" ] && [ -r "${TEMPORARY_FOLDER}" ] && [ -w "${TEMPORARY_FOLDER}" ] && [ -x "${TEMPORARY_FOLDER}" ] ); then
		echo "Failed not create a new folder at ${TEMPORARY_FOLDER}, aborting!"
		exit 1
	else
		echo "Got read, write and execution rights to the ${TEMPORARY_FOLDER} folder, so we can use it to store temporary files."
	fi	
else
	echo "Got read, write and execution rights to the ${TEMPORARY_FOLDER} folder, so we can use it to store temporary files."
fi



# Using buildProject.sh
#   $1 must be the *.DsrProj path, which is relative to the caller location.
#   $2... are variable assignments sent as input to the given project file.

echo "Running buildProject.sh $@"

# Get the build system's folder, where the build system is located
BUILDER_FOLDER=`dirname "$(realpath $0)"`
echo "BUILDER_FOLDER = ${BUILDER_FOLDER}"
BUILDER_EXECUTABLE="${BUILDER_FOLDER}/builder"
echo "BUILDER_EXECUTABLE = ${BUILDER_EXECUTABLE}"

# Check if the build system is compiled
if [ -e "${BUILDER_EXECUTABLE}" ]; then
	echo "Found the build system's binary."
else
	echo "Building the Builder build system for first time use."
	LIBRARY_PATH="$(realpath ${BUILDER_FOLDER}/../../DFPSR)"
	SOURCE_CODE="${BUILDER_FOLDER}/code/main.cpp ${BUILDER_FOLDER}/code/Machine.cpp ${BUILDER_FOLDER}/code/generator.cpp ${BUILDER_FOLDER}/code/analyzer.cpp ${BUILDER_FOLDER}/code/expression.cpp ${LIBRARY_PATH}/collection/collections.cpp ${LIBRARY_PATH}/api/fileAPI.cpp ${LIBRARY_PATH}/api/bufferAPI.cpp ${LIBRARY_PATH}/api/stringAPI.cpp ${LIBRARY_PATH}/api/timeAPI.cpp ${LIBRARY_PATH}/base/SafePointer.cpp ${LIBRARY_PATH}/base/virtualStack.cpp ${LIBRARY_PATH}/base/heap.cpp"
	"${CPP_COMPILER_PATH}" -o "${BUILDER_EXECUTABLE}" ${SOURCE_CODE} -std=c++14 -lstdc++
	if [ $? -eq 0 ]; then
		echo "Completed building the Builder build system."
	else
		echo "Failed building the Builder build system, which is needed to build your project!"
		exit 1
	fi
fi
chmod +x "${BUILDER_EXECUTABLE}"

if [ "$GENERATE_SCRIPT" == "Yes" ]; then
	# Calling the build system with a script path will generate it with compiling and linking commands before executing the result.
	#   Useful for debugging the output when something goes wrong.
	SCRIPT_PATH="${TEMPORARY_FOLDER}/dfpsr_compile.sh"
	echo "Generating ${SCRIPT_PATH} from $1"
	if [ -e "${SCRIPT_PATH}" ]; then
		rm "${SCRIPT_PATH}"
	fi
	"${BUILDER_EXECUTABLE}" "${SCRIPT_PATH}" "$@" "Compiler=${CPP_COMPILER_PATH}";
	if [ -e "${SCRIPT_PATH}" ]; then
		echo "Giving execution permission to ${SCRIPT_PATH}"
		chmod +x "${SCRIPT_PATH}"
		echo "Running ${SCRIPT_PATH}"
		"${SCRIPT_PATH}"
	fi
else
	# Calling the build system with only the temporary folder will call the compiler directly from the build system.
	#   A simpler solution that works with just a single line, once the build system itself has been compiled.
	echo "Generating objects to ${TEMPORARY_FOLDER} from $1"
	"${BUILDER_EXECUTABLE}" "${TEMPORARY_FOLDER}" "$@" "Compiler=${CPP_COMPILER_PATH}";
	if [ $? -eq 0 ]; then
		echo "Finished building."
	else
		echo "Failed building the project!"
		exit 1
	fi
fi
