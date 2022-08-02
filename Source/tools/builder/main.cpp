
// TODO:
//  * Let the build script define a temporary folder path for lazy compilation with reused *.o files.
//    The /tmp folder is erased when shutting down the computer, which would force recompilation of each library after each time the computer has rebooted.
//  * Find a way to check if a file is truly unique using a combination of pathless filenames, content checksums and visibility of surrounding files from the folder.
//    This makes sure that including the same file twice using alternative ways of writing the path can be detected and trigger a warning.
//    Can one convert the file ID from each platform into a string or 64-bit hash sum to quickly make sure that the file is unique?
//  * Create scripts compiling the build system when it does not exist and managing creation of a temporary folder, calling the generated script...
//  * Implement more features for the machine, such as:
//    * Unary negation.
//    * else and elseif cases.
//    * Temporarily letting the theoretical path go into another folder within a scope, similar to if statements but only affecting the path.
//      Like writing (cd path; stmt;) in Bash but with fast parsed Basic-like syntax.

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
			} else {
				String key = string_removeOuterWhiteSpace(string_before(argument, assignmentIndex));
				String value = string_removeOuterWhiteSpace(string_after(argument, assignmentIndex));
				assignValue(settings, key, value);
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
