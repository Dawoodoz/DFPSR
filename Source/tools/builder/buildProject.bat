
rem Local build settings that should be configured before building for the first time.

set CPP_COMPILER_FOLDER=C:\Program\CodeBlocks\MinGW\bin
set CPP_COMPILER_PATH=%CPP_COMPILER_FOLDER%\x86_64-w64-mingw32-g++.exe

rem Change the temporary folder if want generated scripts and objects to go somewhere else.
set TEMPORARY_FOLDER=%TEMP%





@echo off

rem Using buildProject.bat
rem   %1 must be the *.DsrProj path or a folder containing such projects. The path is relative to the caller location.
rem   %2... are variable assignments sent as input to the given project file.
rem   CPP_COMPILER_PATH should be modified if it does not already refer to an installed C++ compiler.

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

echo Change CPP_COMPILER_FOLDER and CPP_COMPILER_PATH in %BUILDER_FOLDER%\buildProject.bat if you are not using %CPP_COMPILER_PATH% as your compiler.

rem Check if the build system is compiled
if exist %BUILDER_EXECUTABLE% (
	echo Found the build system's binary.
) else (
	echo Building the Builder build system for first time use.
	pushd %CPP_COMPILER_FOLDER%
		%CPP_COMPILER_PATH% -o %BUILDER_EXECUTABLE% %BUILDER_SOURCE% -static -static-libgcc -static-libstdc++ -std=c++14 -lstdc++
	popd
	if errorlevel 0 (
		echo Completed building the Builder build system.
	) else (
		echo Failed building the Builder build system, which is needed to build your project!
		pause
		exit /b 1
	)
)

rem Call the build system with a filename for the output script, which is later called.
set SCRIPT_PATH=%TEMPORARY_FOLDER%\dfpsr_compile.bat
echo Generating %SCRIPT_PATH% from %1%
if exist %SCRIPT_PATH% (
	del %SCRIPT_PATH%
)
%BUILDER_EXECUTABLE% %SCRIPT_PATH% %* Compiler=%CPP_COMPILER_PATH% CompileFrom=%CPP_COMPILER_FOLDER%
if exist %SCRIPT_PATH% (
	echo Running %SCRIPT_PATH%
	%SCRIPT_PATH%
)

rem Calling the build system with only the temporary folder will call the compiler directly from the build system.
rem %BUILDER_EXECUTABLE% %TEMPORARY_FOLDER% %* Compiler=%CPP_COMPILER_PATH% CompileFrom=%CPP_COMPILER_FOLDER%

pause
