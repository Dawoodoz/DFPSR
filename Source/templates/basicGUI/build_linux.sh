#!/bin/bash

# Launch the build system with BasicGUI.DsrProj and Linux selected as the platform.
echo "Running build_linux.sh $@"
chmod +x ../../tools/builder/buildProject.sh;
../../tools/builder/buildProject.sh BasicGUI.DsrProj Linux $@;
