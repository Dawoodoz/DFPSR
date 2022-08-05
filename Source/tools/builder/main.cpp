
// Because it would be slow to check if the build system needs to be recompiled every time something uses it,
//   you must manually delete the build system's binary and try to build a project using it after making changes to the builder's source code.
//   Otherwise buildProject.sh will just see that an old version exists and use it.

// TODO:
//  * Let the build script define a temporary folder path for lazy compilation with reused *.o files.
//    The /tmp folder is erased when shutting down the computer, which would force recompilation of each library after each time the computer has rebooted.
//  * Find a way to check if a file is truly unique using a combination of pathless filenames, content checksums and visibility of surrounding files from the folder.
//    This makes sure that including the same file twice using alternative ways of writing the path can be detected and trigger a warning.
//    Can one convert the file ID from each platform into a string or 64-bit hash sum to quickly make sure that the file is unique?
//  * Implement more features for the machine, such as:
//    * Unary negation.
//    * else and elseif cases.
//    * Temporarily letting the theoretical path go into another folder within a scope, similar to if statements but only affecting the path.
//      Like writing (cd path; stmt;) in Bash but with fast parsed Basic-like syntax.
//      The same stack used to store theoretical paths might be useful for else if cases to remember when the scope has already passed a case when not jumping with gotos.
//  * Create portable scripted events for pre-build and post-build, translated into both Batch and Bash.
//    Pre-build can be used to generate and transpile code before compiling.
//    Post-build should be used to execute the resulting program.
//      Optionally with variables from the build script as input arguments.
//  * Should the build system detect the local system automatically, or support cross compilation using a MinGW extension?

#include "../../DFPSR/api/fileAPI.h"
#include "generator.h"

using namespace dsr;

// List dependencies for main.cpp on Linux: ./builder main.cpp --depend
DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	if (args.length() <= 2) {
		printText(U"To use the DFPSR build system, pass a path to the project file and the flags you want assigned before running the build script.\n");
	} else {
		// Get the project file path from the first argument after the program name.
		String projectFilePath = args[1];
		String platform = args[2];
		Machine settings;
		// Begin reading input arguments after the project's path, as named integers assigned to ones.
		// Calling builder with the extra arguments "Graphics" and "Linux" will then create and assign both variables to 1.
		// Other values can be assigned using an equality sign.
		//   Avoid spaces around the equality sign, because quotes are already used for string arguments in assignments.
		for (int a = 2; a < args.length(); a++) {
			String argument = args[a];
			int64_t assignmentIndex = string_findFirst(argument, U'=');
			if (assignmentIndex == -1) {
				assignValue(settings, argument, U"1");
				printText(U"Assigning ", argument, U" to 1 from input argument.\n");
			} else {
				String key = string_removeOuterWhiteSpace(string_before(argument, assignmentIndex));
				String value = string_removeOuterWhiteSpace(string_after(argument, assignmentIndex));
				assignValue(settings, key, value);
				printText(U"Assigning ", key, U" to ", value, U" from input argument.\n");
			}
		}
		// Evaluate compiler settings while searching for source code mentioned in the project and imported headers.
		printText(U"Executing project file from ", projectFilePath, U".\n");
		evaluateScript(settings, projectFilePath);
		// Once we are done finding all source files, we can resolve the dependencies to create a graph connected by indices.
		resolveDependencies();
		if (getFlagAsInteger(settings, U"ListDependencies")) {
			printDependencies();
		}
		generateCompilationScript(settings, file_getAbsoluteParentFolder(projectFilePath));
	}
}
