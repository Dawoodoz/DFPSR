﻿
// Because it would be slow to check if the build system needs to be recompiled every time something uses it,
//   you must manually delete the build system's binary and try to build a project using it after making changes to the builder's source code.
//   Otherwise buildProject.sh will just see that an old version exists and use it.

// TODO:
//  * Create a file with aliases, so that import can use
//      Import <DFPSR>
//    instead of
//      Import "../../DFPSR/DFPSR.DsrHead"
//  * Call the compiler directly when the temp folder is given without any script name.
//    Use it to run multiple instances of the compiler at the same time on different CPU cores.
//  * Improve entropy in checksums using a more advanced algorithm to reduce the risk of conflicts.
//  * Implement more features for the machine, such as:
//    * else and elseif cases.
//    * Temporarily letting the theoretical path go into another folder within a scope, similar to if statements but only affecting the path.
//      Like writing (cd path; stmt;) in Bash but with fast parsed Basic-like syntax.
//      The same stack used to store theoretical paths might be useful for else if cases to remember when the scope has already passed a case when not jumping with gotos.
//  * Create portable scripted events for pre-build and post-build, translated into both Batch and Bash.
//    Pre-build can be used to generate and transpile code before compiling.
//    Post-build should be used to execute the resulting program.
//      Optionally with variables from the build script as input arguments.

/*
Project files:
	Syntax:
		* Assign "10" to variable x:
			x = 10
		* Assign "1" to variable myVariable:
			myVariable
		* Assign b plus c to a:
			a = b + c
		* Assign b minus c to a:
			a = b - c
		* Assign b times c to a:
			a = b * c
		* Assign b divided by c to a:
			a = b / c
		* Concatenate "hello" and " world" into "hello world" in message:
			message = "hello" & " world"
		* If a is less than b or c equals 3 then assign y to z:
			if (a < b) or (c == 3)
				z = y
			end if
		* x is assigned a boolean value telling if the content of a matches "abc". (case sensitive comparison)
			x = a matches "abc"
	Commands:
		* Build all projects in myFolder with the SkipIfBinaryExists flag in arbitrary order before continuing with compilation
			Build "../myFolder" SkipIfBinaryExists
		* Add file.cpp and other implementations found through includes into the list of source code to compile and link.
			Crawl "folder/file.cpp"
	Systems:
		* Linux
			Set to non-zero on Linux or similar operating systems.
		* Windows
			Set to non-zero on MS-Windows.
	Variables:
		* SkipIfBinaryExists, skips building if the binary already exists.
		* Supressed, prevents a compiled program from running after building, which is usually given as an extra argument to Build to avoid launching all programs in a row.
		* ProgramPath, a path to the application to create.
		* Compiler, a path or global alias to the compiler.
		* CompileFrom, from which path should the compiler be executed? Leave empty to use the current directory.
		* Debug, 0 for release, anything else (usually 1) for debug.
		* StaticRuntime, 0 for dynamic runtime linking, anything else (usually 1) for static runtime.
		* Optimization, a natural integer specifying the amount of optimization to apply.
*/

#include "../../DFPSR/api/fileAPI.h"
#include "Machine.h"
#include "expression.h"
#include "generator.h"

using namespace dsr;

// List dependencies for main.cpp on Linux: ./builder main.cpp --depend
DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	if (args.length() <= 1) {
		printText(U"No arguments given to Builder. Starting regression test.\n");
		expression_runRegressionTests();
	} else if (args.length() == 2) {
		printText(U"To use the DFPSR build system, pass a path to a script to generate, a project file or folder containing multiple projects, and the flags you want assigned before building.\n");
		printText(U"To run regression tests, don't pass any argument to the program.\n");
	} else {
		// Get the script's destination path for all projects built during the session as the first argument.
		String scriptPath = args[1];
		String tempFolder = file_getAbsoluteParentFolder(scriptPath);
		// Get the first project file's path, or a folder path containing all projects to build.
		String projectPath = args[2];
		// Read the reas after the project's path, as named integers assigned to ones.
		// Calling builder with the extra arguments will interpret them as variables and mark them as inherited, so that they are passed on to any other projects build from the project file.
		// Other values can be assigned using an equality sign.
		//   Avoid spaces around the equality sign, because quotes are already used for string arguments in assignments.
		Machine settings;
		argumentsToSettings(settings, args, 3);
		// Generate build instructions.
		String executableExtension;
		if (getFlagAsInteger(settings, U"Windows")) {
			executableExtension = U".exe";
		}
		SessionContext buildContext = SessionContext(tempFolder, executableExtension);
		build(buildContext, projectPath, settings);
		// Generate a script to execute.
		// TODO: Store compiler flags in groups of lists to allow taking them directly as program arguments when calling the compiler directly.
		generateCompilationScript(buildContext, scriptPath);
	}
}
