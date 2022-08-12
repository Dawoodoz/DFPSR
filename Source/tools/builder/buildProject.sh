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

CPP_COMPILER_PATH="g++"
echo "Change CPP_COMPILER_PATH in ${BUILDER_FOLDER}/buildProject.sh if you are not using ${CPP_COMPILER_PATH} as your compiler."

# Check if the build system is compiled
if [ -e "${BUILDER_EXECUTABLE}" ]; then
	echo "Found the build system's binary."
else
	echo "Building the Builder build system for first time use."
	LIBRARY_PATH="$(realpath ${BUILDER_FOLDER}/../../DFPSR)"
	SOURCE_CODE="${BUILDER_FOLDER}/code/main.cpp ${BUILDER_FOLDER}/code/Machine.cpp ${BUILDER_FOLDER}/code/generator.cpp ${BUILDER_FOLDER}/code/analyzer.cpp ${BUILDER_FOLDER}/code/expression.cpp ${LIBRARY_PATH}/collection/collections.cpp ${LIBRARY_PATH}/api/fileAPI.cpp ${LIBRARY_PATH}/api/bufferAPI.cpp ${LIBRARY_PATH}/api/stringAPI.cpp ${LIBRARY_PATH}/base/SafePointer.cpp"
	"${CPP_COMPILER_PATH}" -o "${BUILDER_EXECUTABLE}" ${SOURCE_CODE} -std=c++14
	if [ $? -eq 0 ]; then
		echo "Completed building the Builder build system."
	else
		echo "Failed building the Builder build system, which is needed to build your project!"
		exit 1
	fi
fi
chmod +x "${BUILDER_EXECUTABLE}"

# Call the build system with a filename for the output script, which is later given execution permission and called.
SCRIPT_PATH="/tmp/dfpsr_compile.sh"
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
