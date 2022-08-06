
#ifndef DSR_BUILDER_GENERATOR_MODULE
#define DSR_BUILDER_GENERATOR_MODULE

#include "../../DFPSR/api/fileAPI.h"
#include "Machine.h"

using namespace dsr;

// Analyze using calls from the machine
void analyzeFromFile(const ReadableString& entryPath);
// Call from main when done analyzing source files
void resolveDependencies();

// Visualize
void printDependencies();

// Generate
/*
Setting variables:
	* ScriptPath, a path to the script to be generated and saved. The extension of the filename decides which type of code to generate.
	* ProgramPath, a path to the application to create.
	* Compiler, a path or global alias to the compiler.
	* CompileFrom, from which path should the compiler be executed? Leave empty to use the current directory.
	* Debug, 0 for release, anything else (usually 1) for debug.
	* StaticRuntime, 0 for dynamic runtime linking, anything else (usually 1) for static runtime.
	* Optimization, a natural integer specifying the amount of optimization to apply.
	
*/
void generateCompilationScript(const Machine &settings, const ReadableString& projectPath);

#endif
