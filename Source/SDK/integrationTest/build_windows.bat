@echo off

rem Launch the build system with IntegrationTest.DsrProj and Windows selected as the platform.

echo "Running build_windows.bat %@%
..\..\tools\builder\buildProject.bat IntegrationTest.DsrProj Windows %@%
