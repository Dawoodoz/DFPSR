#!/bin/bash

# Make sure to compile the documentation generator using build_linux.sh or build_macos.sh before calling the program with, input, output and resource paths.

# Generate documentation from the folder ./Input to the Doc folder outside at .., using the style in ./Resources.
./generator ./Input .. ./Resources
