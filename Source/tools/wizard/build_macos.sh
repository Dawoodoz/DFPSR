# Hack to use X11 on MacOS
export CPATH=/opt/homebrew/include:$CPATH
export LIBRARY_PATH=/opt/homebrew/lib:$LIBRARY_PATH

# Launch the build system with Wizard.DsrProj and MacOS selected as the platform.
echo "Running build_macos.sh $@"
chmod +x ../builder/buildProject.sh;
../builder/buildProject.sh Wizard.DsrProj MacOS $@;
