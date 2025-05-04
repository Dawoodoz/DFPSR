#!/bin/bash

# Launch the build system with Clone.DsrProj and Linux selected as the platform.
echo "Running build_linux.sh $@"
chmod +x ../../builder/buildProject.sh;
../../builder/buildProject.sh Clone.DsrProj Linux $@;
