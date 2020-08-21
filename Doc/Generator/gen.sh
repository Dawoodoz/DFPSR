#!/bin/bash

# The root of the source files
SOURCE_FOLDER=$1
# The target folder where the library will be created
TARGET_FOLDER=$2
# The resource folder where styles are found
RESOURCE_FOLDER=$3

# Argument: $1 as the folder to generate HTML from
generateInFolder() {
	# Convert all the text files into HTML
	for file in "$1"/*.txt
	do
		[ -e $file ] || continue
		# Get name without path
		name=${file##*/}
		# Get name without extension nor path
		base=${name%.txt}
		./generator ${file} ${TARGET_FOLDER}/${base}.html ${RESOURCE_FOLDER}
		if [ $? -ne 0 ]
		then
			echo "Failed to convert ${file}!"
			exit 1
		fi
	done
	# Recursively compile other folders
	for folder in "$1"/*
	do
		if [ -d "$folder" ]
		then
			generateInFolder "$folder"
		fi
	done
}

echo "Generating HTML from $SOURCE_FOLDER to $TARGET."
generateInFolder ${SOURCE_FOLDER}
