#!/bin/bash

# Launch the build system with Basic3D.DsrProj and Linux selected as the platform.
echo "Running build_linux.sh $@"
chmod +x ../../tools/builder/buildProject.sh;
../../tools/builder/buildProject.sh Basic3D.DsrProj Linux $@;
