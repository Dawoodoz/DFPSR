#!/bin/bash

# run-simd-tests.sh
# Build and run only the SIMD test modules (BruteSimdTest, SimdTest, VectorTest).
# Intended for CI on non-x86 targets where running the full suite under
# emulation would be too slow. Mirrors the build flow of test.sh.

TEST_FOLDER=$(dirname "$(realpath "$0")")
cd "${TEST_FOLDER}" || exit 1

ROOT_PATH=..
TEMP_ROOT=${ROOT_PATH}/../../temporary
CPP_VERSION=-std=c++14
MODE="-DDEBUG"
DEBUGGER="-g"
O_LEVEL=-O2

# SIMD target flag. Default: enable LSX on LoongArch.
# Overridable via SIMD_FLAG env var. Falls back to -msx for older GCC.
if [ -z "${SIMD_FLAG}" ]; then
	if echo | g++ -mlsx -dM -E -x c++ - >/dev/null 2>&1; then
		SIMD_FLAG="-mlsx"
	elif echo | g++ -msx -dM -E -x c++ - >/dev/null 2>&1; then
		SIMD_FLAG="-msx"
	else
		SIMD_FLAG="-march=native"
	fi
fi
echo "Using SIMD_FLAG = ${SIMD_FLAG}"

COMPILER_FLAGS="${MODE} ${DEBUGGER} ${SIMD_FLAG} ${CPP_VERSION} ${O_LEVEL}"

chmod +x "${ROOT_PATH}/tools/buildScripts/build.sh";
"${ROOT_PATH}/tools/buildScripts/build.sh" "NONE" "NONE" "${ROOT_PATH}" "${TEMP_ROOT}" "NONE" "${COMPILER_FLAGS}";
if [ $? -ne 0 ]; then
	exit 1
fi

# Get the specific temporary sub-folder for the compilation settings
# (must match the substitution used inside build.sh)
TEMP_SUB="${COMPILER_FLAGS// /_}"
TEMP_SUB=$(echo $TEMP_SUB | tr "+" "p")
TEMP_SUB=$(echo $TEMP_SUB | tr -d "=-")
TEMP_DIR=${TEMP_ROOT}/${TEMP_SUB}

# Build empty backends to prevent getting linker errors
g++ ${CPP_VERSION} ${MODE} ${DEBUGGER} ${SIMD_FLAG} -c ${ROOT_PATH}/windowManagers/NoWindow.cpp -o ${TEMP_DIR}/NoWindow.o;
if [ $? -ne 0 ]; then
	exit 1
fi
g++ ${CPP_VERSION} ${MODE} ${DEBUGGER} ${SIMD_FLAG} -c ${ROOT_PATH}/soundManagers/NoSound.cpp -o ${TEMP_DIR}/NoSound.o;
if [ $? -ne 0 ]; then
	exit 1
fi

TESTS="${TESTS:-BruteSimdTest SimdTest VectorTest}"

for base in ${TESTS}; do
	file="./tests/${base}.cpp"
	[ -e $file ] || { echo "Missing ${file}"; exit 1; }
	# Remove previous test case
	rm -f ${TEMP_DIR}/*_test.o;
	rm -f ${TEMP_DIR}/application;
	# Compile test case that defines main
	echo "Compiling ${base}.cpp";
	g++ ${CPP_VERSION} ${MODE} ${DEBUGGER} ${SIMD_FLAG} -c ${file} -o ${TEMP_DIR}/${base}_test.o;
	if [ $? -ne 0 ]; then
		exit 1
	fi
	# Linking with frameworks
	echo "Linking ${base}.cpp";
	g++ ${TEMP_DIR}/*.o ${TEMP_DIR}/*.a -lm -pthread -o ${TEMP_DIR}/application;
	if [ $? -ne 0 ]; then
		exit 1
	fi
	# Run the test case
	echo "Executing ${base}.cpp";
	./${TEMP_DIR}/application --path ./tests;
	if [ $? -eq 0 ]; then
		echo "Passed ${base}!";
	else
		echo "Failed ${base}!";
		exit 1
	fi
done

echo "All SIMD tests passed."
