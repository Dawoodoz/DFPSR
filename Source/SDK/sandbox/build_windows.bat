@echo off

rem Launch the build system with Sandbox.DsrProj and Windows selected as the platform.

echo "Running build_windows.bat %@%
..\..\tools\builder\buildProject.bat Sandbox.DsrProj Windows %@%
