
#ifndef DSR_BUILDER_ANALYZER_MODULE
#define DSR_BUILDER_ANALYZER_MODULE

#include "../../../DFPSR/api/fileAPI.h"
#include "builderTypes.h"

using namespace dsr;

// Analyze using calls from the machine
void analyzeFromFile(ProjectContext &context, const ReadableString& entryPath);
// Call from main when done analyzing source files
void resolveDependencies(ProjectContext &context);

// Visualize
void printDependencies(ProjectContext &context);

// Build anything in projectPath.
void build(SessionContext &output, const ReadableString &projectPath, Machine &settings);

// Build the project in projectFilePath.
// Settings must be taken by value to prevent side-effects from spilling over between different scripts.
void buildProject(SessionContext &output, const ReadableString &projectFilePath, Machine settings);

// Build all projects in projectFolderPath.
void buildProjects(SessionContext &output, const ReadableString &projectFolderPath, Machine &settings);

void gatherBuildInstructions(SessionContext &output, ProjectContext &context, Machine &settings, ReadableString programPath);

#endif
