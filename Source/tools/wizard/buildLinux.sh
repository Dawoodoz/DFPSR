#!/bin/bash

# Launch the build system with main.DsrProj and Linux selected as the platform.
echo "Running buildLinux.sh $@"
chmod +x ../builder/buildProject.sh;
../builder/buildProject.sh ./main.DsrProj Linux;
