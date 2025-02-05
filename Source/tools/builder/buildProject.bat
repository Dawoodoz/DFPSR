@echo off

setlocal enabledelayedexpansion

rem Local build settings.
set GENERATE_SCRIPT=Yes
rem set GENERATE_SCRIPT=No
set TEMPORARY_FOLDER=%TEMP%
set COMPILER_NAME=g++





rem Get this script's folder.
set BUILDER_FOLDER=%~dp0
echo BUILDER_FOLDER = %BUILDER_FOLDER%

rem Show which versions of the compiler are installed.
echo Installed %COMPILER_NAME% compilers:
where %COMPILER_NAME%

rem Select the first instance.
for /f "delims=" %%i in ('where %COMPILER_NAME% 2^>nul') do (
	set CPP_COMPILER_PATH=%%i
	goto :found_compiler
)
:found_compiler
rem Abort if none could be found.
if not exist !CPP_COMPILER_PATH! (
	echo Could not find !COMPILER_NAME!.
	exit /b 1
)
echo CPP_COMPILER_PATH = !CPP_COMPILER_PATH!

rem Get the compiler's folder from the compiler's path.
set CPP_COMPILER_FOLDER=!~dpCPP_COMPILER_PATH!
echo CPP_COMPILER_FOLDER = !CPP_COMPILER_FOLDER!

rem Using buildProject.bat
rem   %1 must be the *.DsrProj path or a folder containing such projects. The path is relative to the caller location.
rem   %2... are variable assignments sent as input to the given project file.

echo Running buildProject.bat %*

rem Get the build system's folder, where the build system is located.
set BUILDER_FOLDER=%~dp0%
echo BUILDER_FOLDER = %BUILDER_FOLDER%
set BUILDER_EXECUTABLE=%BUILDER_FOLDER%builder.exe
echo BUILDER_EXECUTABLE = %BUILDER_EXECUTABLE%
set DFPSR_LIBRARY=%BUILDER_FOLDER%..\..\DFPSR
echo DFPSR_LIBRARY = %DFPSR_LIBRARY%
set BUILDER_SOURCE=%BUILDER_FOLDER%\code\main.cpp %BUILDER_FOLDER%\code\Machine.cpp %BUILDER_FOLDER%\code\generator.cpp %BUILDER_FOLDER%\code\analyzer.cpp %BUILDER_FOLDER%\code\expression.cpp %DFPSR_LIBRARY%\collection\collections.cpp %DFPSR_LIBRARY%\api\fileAPI.cpp %DFPSR_LIBRARY%\api\bufferAPI.cpp %DFPSR_LIBRARY%\api\stringAPI.cpp %DFPSR_LIBRARY%\api\timeAPI.cpp %DFPSR_LIBRARY%\base\SafePointer.cpp %DFPSR_LIBRARY%\base\virtualStack.cpp %DFPSR_LIBRARY%\base\heap.cpp
echo BUILDER_SOURCE = %BUILDER_SOURCE%

rem Check if the build system is compiled
if exist "%BUILDER_EXECUTABLE%" (
	echo Found the build system's binary.
) else (
	echo Building the Builder build system for first time use.
	pushd %CPP_COMPILER_FOLDER%
		%CPP_COMPILER_PATH% -o %BUILDER_EXECUTABLE% %BUILDER_SOURCE% -static -static-libgcc -static-libstdc++ -std=c++14 -lstdc++
		if errorlevel 0 (
			echo Completed building the Builder build system.
		) else (
			echo Failed building the Builder build system, which is needed to build your project!
			exit /b 1
		)
	popd
)

if "!GENERATE_SCRIPT!"=="Yes" (
	rem Calling the build system with a script path will generate the script for all actions.
	set SCRIPT_PATH=%TEMPORARY_FOLDER%\dfpsr_compile.bat
	echo Generating !SCRIPT_PATH! from %1%
	if exist "!SCRIPT_PATH!" (
		del "!SCRIPT_PATH!"
	)
	!BUILDER_EXECUTABLE! "!SCRIPT_PATH!" %* "Compiler=!CPP_COMPILER_PATH!" "CompileFrom=!CPP_COMPILER_FOLDER!"
	if exist "!SCRIPT_PATH!" (
		echo Running !SCRIPT_PATH!
		call "!SCRIPT_PATH!"
		echo Done calling !SCRIPT_PATH!
	)
) else (
	rem Calling the build system with only the temporary folder will call the compiler directly from the build system.
	echo No script path provided. Builder will call !COMPILER_NAME! directly instead of generating a script.
	!BUILDER_EXECUTABLE! "%TEMPORARY_FOLDER%" %* Compiler=!CPP_COMPILER_PATH! CompileFrom=!CPP_COMPILER_FOLDER!
)

endlocal
