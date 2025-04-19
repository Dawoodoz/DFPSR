
# Launch the build system with Wizard.DsrProj and MacOS selected as the platform.
echo "Running build_macos.sh $@"
chmod +x ../builder/buildProject.sh;
../builder/buildProject.sh Wizard.DsrProj MacOS $@;
