#!/bin/bash

# Load arguments into named variables in order
TEMP_DIR=$1 # Which folder to create or clear

# Create a temporary folder
mkdir -p ${TEMP_DIR}
# Remove objects, but keep libraries
rm -f ${TEMP_DIR}/*.o
