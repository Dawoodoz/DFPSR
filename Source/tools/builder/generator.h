
#ifndef DSR_BUILDER_GENERATOR_MODULE
#define DSR_BUILDER_GENERATOR_MODULE

#include "../../DFPSR/api/fileAPI.h"
#include "Machine.h"

using namespace dsr;

// Analyze using calls from the machine
void analyzeFromFile(ProjectContext &context, const ReadableString& entryPath);
// Call from main when done analyzing source files
void resolveDependencies(ProjectContext &context);

// Visualize
void printDependencies(ProjectContext &context);

// Generate
void generateCompilationScript(ScriptTarget &output, ProjectContext &context, const Machine &settings, ReadableString programPath);

#endif
