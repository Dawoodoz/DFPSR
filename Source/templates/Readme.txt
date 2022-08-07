To get started with your own project:
* Copy one of these folders into a location with stable relative access to the DFPSR library.
	You can also place DFPSR in a standard path, refer to it using absolute paths and always have your code in the same location.
	On Linux, you can also place a symbolic link right next to your projects and use relative paths using the symbolic link as a folder name.
* Change #include "../../DFPSR/includeFramework.h" in main.cpp to find the header from the new location.
* Change path for tools/builder/buildProject.sh in build_linux.sh, and tools/builder/buildProject.bat in build_windows.bat,
	so that they give your project file to the build system.
* Change path for DFPSR/DFPSR.DsrHead in main.DsrProj if imported in your project.

Build on Linux:
* Give execution permission using:
	chmod +x build_linux.sh
* Build using:
	./build_linux.sh
"./" for the relative path is needed to distinguish the path from global aliases for the first argument, but not needed when given as input to a program.

Build on Windows:
* Install the MinGW edition of CodeBlocks.
	Both to get an IDE that can be used for debugging, and because it's the easiest way to install a GNU compiler on MS-Windows.
* Update CPP_COMPILER_PATH and CPP_COMPILER_FOLDER to reference where your C++ compiler is located.
	If you installed CodeBlocks in C:\Program, it might work without any changes.
	CPP_COMPILER_PATH is the absolute path to the specific compiler's binary.
	CPP_COMPILER_FOLDER is the folder to launch the compiler from.
	Any dynamic dependency being used by the compiler that is not registered (regsvr32 if it's self registering),
	will be loaded from the current directory and not the compiler's own folder (since Windows XP).
	Therefore the build system will temporarily enter the folder specified by CPP_COMPILER_FOLDER as the CompileFrom flag,
	so that the compiler's dependencies can be loaded from the compiler's own folder where DLL files are usually stored.
* Double click on buildProject.bat to build and run.
