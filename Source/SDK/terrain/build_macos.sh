# Hack to use X11 on MacOS
export CPATH=/opt/homebrew/include:$CPATH
export LIBRARY_PATH=/opt/homebrew/lib:$LIBRARY_PATH

# Launch the build system with Terrain.DsrProj and MacOS selected as the platform.
echo "Running build_linux.sh $@"
chmod +x ../../tools/builder/buildProject.sh;
../../tools/builder/buildProject.sh Terrain.DsrProj MacOS $@;
