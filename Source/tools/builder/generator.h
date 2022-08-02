
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
enum class ScriptLanguage {
	Unknown,
	Batch,
	Bash
};
void generateCompilationScript(const Machine &settings, const ReadableString& projectPath);

#endif
