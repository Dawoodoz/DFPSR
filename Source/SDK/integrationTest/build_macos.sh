
# Launch the build system with IntegrationTest.DsrProj and MacOS selected as the platform.
echo "Running build_macos.sh $@"
chmod +x ../../tools/builder/buildProject.sh;
../../tools/builder/buildProject.sh IntegrationTest.DsrProj MacOS $@;
