#!/bin/bash

LIBRARY_PATH=../../DFPSR
DEPENDENCIES="${LIBRARY_PATH}/collection/collections.cpp ${LIBRARY_PATH}/api/fileAPI.cpp ${LIBRARY_PATH}/api/bufferAPI.cpp ${LIBRARY_PATH}/api/stringAPI.cpp ${LIBRARY_PATH}/base/SafePointer.cpp Machine.cpp generator.cpp"

# Compile the analyzer
g++ main.cpp -o builder ${DEPENDENCIES} -std=c++14;

# Compile the wizard project to test if the build system works
echo "Generating dfpsr_compile.sh from Wizard.DsrProj"
./builder ../wizard/Wizard.DsrProj Graphics Sound Linux ScriptPath="/tmp/dfpsr_compile.sh";
echo "Generating dfpsr_win32_test.bat from Wizard.DsrProj"
./builder ../wizard/Wizard.DsrProj Graphics Sound Windows ScriptPath="/tmp/dfpsr_win32_test.bat";
echo "Running compile.sh"
chmod +x /tmp/dfpsr_compile.sh
/tmp/dfpsr_compile.sh
