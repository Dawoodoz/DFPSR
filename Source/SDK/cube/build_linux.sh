#!/bin/bash

# Launch the build system with Cube.DsrProj and Linux selected as the platform.
echo "Running build_linux.sh $@"
chmod +x ../../tools/builder/buildProject.sh;
../../tools/builder/buildProject.sh Cube.DsrProj Linux $@;
