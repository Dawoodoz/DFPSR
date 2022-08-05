#!/bin/bash

# Launch the build system with main.DsrProj and Linux selected as the platform.
echo "Running build_linux.sh $@"
chmod +x ../builder/buildProject.sh;
../builder/buildProject.sh main.DsrProj Linux $@;
