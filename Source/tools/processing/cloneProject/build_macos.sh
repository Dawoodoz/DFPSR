#!/bin/bash

# Launch the build system with Clone.DsrProj and MacOS selected as the platform.
echo "Running build_macos.sh $@"
chmod +x ../../builder/buildProject.sh;
../../builder/buildProject.sh Clone.DsrProj MacOS $@;
