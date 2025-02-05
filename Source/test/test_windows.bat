@echo off

rem This script can be called from any path, as long as the first argument sais where this script is located.

echo Starting test on MS-Windows.

rem Get the test folder's path from the called path.
set TEST_FOLDER=%~dp0
echo TEST_FOLDER = %TEST_FOLDER%

set PROJECT_BUILD_SCRIPT=%TEST_FOLDER%..\tools\builder\buildProject.bat
echo PROJECT_BUILD_SCRIPT = %PROJECT_BUILD_SCRIPT%

set PROJECT_FILE=%TEST_FOLDER%TestCaller.DsrProj
echo PROJECT_FILE = %PROJECT_FILE%

rem Build TestCaller and all its tests. 
call "%PROJECT_BUILD_SCRIPT%" "%PROJECT_FILE%" Windows %@%
if errorlevel 0 (
	echo Done building TestCaller.
) else (
	echo Failed building TestCaller.
	exit /b 1
)

rem Call TestCaller with a path to the folder containing tests.
echo Starting tests.
call "%TEST_FOLDER%\TestCaller.exe" --test "%TEST_FOLDER%\tests"
if errorlevel 0 (
	echo Done running tests.
) else (
	echo Failed running tests.
	exit /b 1
)
