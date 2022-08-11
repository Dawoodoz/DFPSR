
#ifndef DSR_BUILDER_MACHINE_MODULE
#define DSR_BUILDER_MACHINE_MODULE

#include "../../DFPSR/api/stringAPI.h"

using namespace dsr;

struct Flag {
	// Flags created externally using argumentsToSettings from either the command line or another project, will be marked as inherited and given to the next call.
	bool inherited;
	dsr::String key, value;
	Flag() {}
	Flag(const dsr::ReadableString &key, const dsr::ReadableString &value, bool inherited)
	: key(key), value(value), inherited(inherited) {}
};

struct Machine {
	List<Flag> variables;
	List<String> compilerFlags;
	List<String> linkerFlags;
	List<String> crawlOrigins;
	// When activeStackDepth < currentStackDepth, we are skipping false cases.
	int64_t currentStackDepth = 0; // How many scopes we are inside of, from the root script including all the others.
	int64_t activeStackDepth = 0;
};

enum class Extension {
	Unknown, H, Hpp, C, Cpp
};

enum class ScriptLanguage {
	Unknown,
	Batch,
	Bash
};

Extension extensionFromString(const ReadableString& extensionName);

struct Connection {
	String path;
	int64_t lineNumber = -1;
	int64_t dependencyIndex = -1;
	Connection(const ReadableString& path)
	: path(path) {}
	Connection(const ReadableString& path, int64_t lineNumber)
	: path(path), lineNumber(lineNumber) {}
};

struct Dependency {
	String path;
	Extension extension;
	uint64_t contentChecksum;
	bool visited; // Used to avoid infinite loops while traversing dependencies.
	List<Connection> links; // Depends on having these linked after compiling.
	List<Connection> includes; // Depends on having these included in pre-processing.
	Dependency(const ReadableString& path, Extension extension, uint64_t contentChecksum)
	: path(path), extension(extension), contentChecksum(contentChecksum) {}
};

struct ProjectContext {
	//Machine settings;
	List<Dependency> dependencies;
	ProjectContext() {}
};

struct SourceObject {
	uint64_t identityChecksum = 0; // Identification number for the object's name.
	uint64_t combinedChecksum = 0; // Combined content of the source file and all included headers recursively.
	String sourcePath, objectPath, generatedCompilerFlags, compilerName, compileFrom;
	SourceObject(uint64_t identityChecksum, uint64_t combinedChecksum, const ReadableString& sourcePath, const ReadableString& objectPath, const ReadableString& generatedCompilerFlags, const ReadableString& compilerName, const ReadableString& compileFrom)
	: identityChecksum(identityChecksum), combinedChecksum(combinedChecksum), sourcePath(sourcePath), objectPath(objectPath), generatedCompilerFlags(generatedCompilerFlags), compilerName(compilerName), compileFrom(compileFrom) {}
};

struct LinkingStep {
	String compilerName, compileFrom, binaryName;
	List<String> linkerFlags;
	List<int64_t> sourceObjectIndices;
	bool executeResult;
	LinkingStep(const ReadableString &compilerName, const ReadableString &compileFrom, const ReadableString &binaryName, const List<String> &linkerFlags, const List<int64_t> &sourceObjectIndices, bool executeResult)
	: compilerName(compilerName), compileFrom(compileFrom), binaryName(binaryName), linkerFlags(linkerFlags), sourceObjectIndices(sourceObjectIndices), executeResult(executeResult) {}
};

struct SessionContext {
	String tempPath;
	String executableExtension;
	List<SourceObject> sourceObjects;
	List<LinkingStep> linkerSteps;
	SessionContext(const ReadableString &tempPath, const ReadableString &executableExtension)
	: tempPath(tempPath), executableExtension(executableExtension) {}
};

// Returns the first case insensitive match for key in target, or -1 if not found.
int64_t findFlag(const Machine &target, const dsr::ReadableString &key);
// Returns the value of key in target, or defaultValue if not found.
ReadableString getFlag(const Machine &target, const dsr::ReadableString &key, const dsr::ReadableString &defaultValue);
// Returns the value of key in target, defaultValue if not found, or 0 if not an integer.
int64_t getFlagAsInteger(const Machine &target, const dsr::ReadableString &key, int64_t defaultValue = 0);

// Assigns value to key in target. Allocates key in target if it does not already exist.
void assignValue(Machine &target, const dsr::ReadableString &key, const dsr::ReadableString &value, bool inherited);

// Modifies the flags in target, while listing source files to context, using the script in scriptPath.
// Recursively including other scripts using the script's folder as the origin for relative paths.
void evaluateScript(SessionContext &output, Machine &target, const ReadableString &scriptPath);

// Build anything in projectPath.
void build(SessionContext &output, const ReadableString &projectPath, Machine &settings);

// Build the project in projectFilePath.
// Settings must be taken by value to prevent side-effects from spilling over between different scripts.
void buildProject(SessionContext &output, const ReadableString &projectFilePath, Machine settings);

// Build all projects in projectFolderPath.
void buildProjects(SessionContext &output, const ReadableString &projectFolderPath, Machine &settings);
void argumentsToSettings(Machine &settings, const List<String> &arguments, int64_t firstArgument);

#endif
