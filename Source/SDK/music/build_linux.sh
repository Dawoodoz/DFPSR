#!/bin/bash

echo "Running build_linux.sh $@"
chmod +x ../../tools/builder/buildProject.sh;
../../tools/builder/buildProject.sh Music.DsrProj Linux $@;
