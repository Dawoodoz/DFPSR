
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
void gatherBuildInstructions(SessionContext &output, ProjectContext &context, Machine &settings, ReadableString programPath);
void generateCompilationScript(SessionContext &output, const ReadableString &scriptPath);

#endif
