#!/bin/bash

# run-loongarch-simd-tests.sh
# Build and run only the SIMD test modules (BruteSimdTest, SimdTest, VectorTest).
#
# This script exists specifically for the LoongArch64 (LSX) CI job, which runs
# inside a QEMU-emulated loongarch64 container (uraimo/run-on-arch-action).
# It is NOT a replacement for test.sh; it works around limitations of that
# emulated environment, where test.sh cannot be used as-is:
#
#   1. test.sh hardcodes -march=native. Under qemu-user the compiler sees the
#      HOST CPU configuration (/proc/cpuinfo of the x86_64 runner), so
#      -march=native either errors out or silently drops LSX — the opposite of
#      what we want to test. We probe -mlsx / -msx explicitly instead
#      (overridable via SIMD_FLAG).
#   2. The full suite (~30 test modules) is far too slow under TCG emulation,
#      and most modules are unrelated to the SIMD backend. Only the three
#      headless compute tests are run here (overridable via TESTS).
#   3. test.sh re-runs failures under gdb, which is not installed in the CI
#      container. This script simply fails with a non-zero exit code.
#
# Correctness coverage only: timings under TCG emulation are not meaningful.

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
