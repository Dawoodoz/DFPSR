
# Launch the build system with Camera.DsrProj and MacOS selected as the platform.
echo "Running build_macos.sh $@"
chmod +x ../../tools/builder/buildProject.sh;
../../tools/builder/buildProject.sh Camera.DsrProj MacOS $@;
