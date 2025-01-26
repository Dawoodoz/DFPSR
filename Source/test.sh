#!/bin/bash

ROOT_PATH=.
TEMP_ROOT=${ROOT_PATH}/../../temporary
CPP_VERSION=-std=c++14
MODE="-DDEBUG"
DEBUGGER="-g"
O_LEVEL=-O2

chmod +x ${ROOT_PATH}/tools/build.sh;
${ROOT_PATH}/tools/build.sh "NONE" "NONE" "${ROOT_PATH}" "${TEMP_ROOT}" "NONE" "${MODE} ${DEBUGGER} ${CPP_VERSION} ${O_LEVEL}";
if [ $? -ne 0 ]
then
	exit 1
fi

# Get the specific temporary sub-folder for the compilation settings
TEMP_SUB="${MODE}_${DEBUGGER}_${CPP_VERSION}_${O_LEVEL}"
TEMP_SUB=$(echo $TEMP_SUB | tr "+" "p")
TEMP_SUB=$(echo $TEMP_SUB | tr -d " =-")
TEMP_DIR=${TEMP_ROOT}/${TEMP_SUB}

for file in ./test/tests/*.cpp; do
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
	g++ ${CPP_VERSION} ${MODE} ${DEBUGGER} -c ${file} -o ${TEMP_DIR}/${base}_test.o;
	# Linking with frameworks
	echo "Linking ${name}";
	g++ ${TEMP_DIR}/*.o ${TEMP_DIR}/*.a -lm -pthread -o ${TEMP_DIR}/application;
	# Run the test case
	echo "Executing ${name}";
	./${TEMP_DIR}/application;
	if [ $? -eq 0 ]
	then
		echo "Passed ${name}!";
	else
		echo "Failed ${name}!";
		# Re-run with a memory debugger.
		gdb ./${TEMP_DIR}/application;
		break;
	fi
done

