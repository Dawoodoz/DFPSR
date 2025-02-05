#!/bin/bash

# This script can be called from any path, as long as the first argument sais where this script is located.

TEST_FOLDER=`dirname "$(realpath $0)"`
echo TEST_FOLDER = "${TEST_FOLDER}"

PROJECT_BUILD_SCRIPT="${TEST_FOLDER}/../tools/builder/buildProject.sh"
echo PROJECT_BUILD_SCRIPT = "${PROJECT_BUILD_SCRIPT}"

PROJECT_FILE="${TEST_FOLDER}/TestCaller.DsrProj"
echo PROJECT_FILE = "${PROJECT_FILE}"

# Give execution rights.
chmod +x "${PROJECT_BUILD_SCRIPT}";

# Build TestCaller and all its tests. 
"${PROJECT_BUILD_SCRIPT}" "${PROJECT_FILE}" MacOS $@;
if [ $? -ne 0 ]
then
	exit 1
fi

# Call TestCaller with a path to the folder containing tests.
"${TEST_FOLDER}/TestCaller" "--test" "${TEST_FOLDER}/tests"
