
# Launch the build system with Music.DsrProj and MacOS selected as the platform.
echo "Running build_linux.sh $@"
chmod +x ../../tools/builder/buildProject.sh;
../../tools/builder/buildProject.sh Music.DsrProj MacOS $@;
