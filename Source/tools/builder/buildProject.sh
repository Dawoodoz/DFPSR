
# Local build settings that should be configured before building for the first time.

# Make sure to erase all objects in the temporary folder before changing compiler.

# Compile using GCC's C++ compiler
CPP_COMPILER_PATH="/usr/bin/g++"

# Compile using CLANG (Needs a 'Link "stdc++"' command in each project)
#CPP_COMPILER_PATH="/usr/bin/clang"

echo "Change CPP_COMPILER_PATH in ${BUILDER_FOLDER}/buildProject.sh if you are not using ${CPP_COMPILER_PATH} as your compiler."

# Change TEMPORARY_FOLDER if you don't want to recompile everything after each reboot, or your operating system has a different path to the temporary folder.
TEMPORARY_FOLDER="/tmp"

# Select build method. (Not generating a script requires having the full path of the compiler.)
#GENERATE_SCRIPT="Yes"
GENERATE_SCRIPT="No"





# Using buildProject.sh
#   $1 must be the *.DsrProj path, which is relative to the caller location.
#   $2... are variable assignments sent as input to the given project file.
#   CPP_COMPILER_PATH should be modified if it does not already refer to an installed C++ compiler.

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
	SOURCE_CODE="${BUILDER_FOLDER}/code/main.cpp ${BUILDER_FOLDER}/code/Machine.cpp ${BUILDER_FOLDER}/code/generator.cpp ${BUILDER_FOLDER}/code/analyzer.cpp ${BUILDER_FOLDER}/code/expression.cpp ${LIBRARY_PATH}/collection/collections.cpp ${LIBRARY_PATH}/api/fileAPI.cpp ${LIBRARY_PATH}/api/bufferAPI.cpp ${LIBRARY_PATH}/api/stringAPI.cpp ${LIBRARY_PATH}/api/timeAPI.cpp ${LIBRARY_PATH}/base/SafePointer.cpp"
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
fi

