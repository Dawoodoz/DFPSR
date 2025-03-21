#!/bin/bash

ROOT_PATH=..
TEMP_ROOT=${ROOT_PATH}/../../temporary
CPP_VERSION=-std=c++14
MODE="-DDEBUG"
DEBUGGER="-g"
SIMD="-march=native"
O_LEVEL=-O2

chmod +x ${ROOT_PATH}/tools/buildScripts/build.sh;
${ROOT_PATH}/tools/buildScripts/build.sh "NONE" "NONE" "${ROOT_PATH}" "${TEMP_ROOT}" "NONE" "${MODE} ${DEBUGGER} ${SIMD} ${CPP_VERSION} ${O_LEVEL}";
if [ $? -ne 0 ]
then
	exit 1
fi

# Get the specific temporary sub-folder for the compilation settings
TEMP_SUB="${MODE}_${DEBUGGER}_${SIMD}_${CPP_VERSION}_${O_LEVEL}"
TEMP_SUB=$(echo $TEMP_SUB | tr "+" "p")
TEMP_SUB=$(echo $TEMP_SUB | tr -d " =-")
TEMP_DIR=${TEMP_ROOT}/${TEMP_SUB}

# Build empty backends to prevent getting linker errors
g++ ${CPP_VERSION} ${MODE} ${DEBUGGER} ${SIMD} -c ${ROOT_PATH}/windowManagers/NoWindow.cpp -o ${TEMP_DIR}/NoWindow.o;
if [ $? -ne 0 ]
then
	exit 1
fi
g++ ${CPP_VERSION} ${MODE} ${DEBUGGER} ${SIMD} -c ${ROOT_PATH}/soundManagers/NoSound.cpp -o ${TEMP_DIR}/NoSound.o;
if [ $? -ne 0 ]
then
	exit 1
fi

for file in ./tests/*.cpp; do
	[ -e $file ] || continue
	# Get name without path
	name=${file##*/};
	# Get name without extension nor path
	base=${name%.cpp};
	# Remove previous test case
	rm -f ${TEMP_DIR}/*_test.o;
	rm -f ${TEMP_DIR}/application;
	# Compile test case that defines main
	echo "Compiling ${name}";
	g++ ${CPP_VERSION} ${MODE} ${DEBUGGER} ${SIMD} -c ${file} -o ${TEMP_DIR}/${base}_test.o;
	if [ $? -ne 0 ]
	then
		exit 1
	fi
	# Linking with frameworks
	echo "Linking ${name}";
	g++ ${TEMP_DIR}/*.o ${TEMP_DIR}/*.a -lm -pthread -o ${TEMP_DIR}/application;
	if [ $? -ne 0 ]
	then
		exit 1
	fi
	# Run the test case
	echo "Executing ${name}";
	./${TEMP_DIR}/application --path ./tests;
	if [ $? -eq 0 ]
	then
		echo "Passed ${name}!";
	else
		echo "Failed ${name}!";
		# Re-run with a memory debugger.
		gdb -ex "run" -ex "bt" -ex "quit" --args ./${TEMP_DIR}/application --path ./tests;
		exit 1
	fi
done

