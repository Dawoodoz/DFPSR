
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
	List<String> compilerFlags, linkerFlags;
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

struct ScriptTarget {
	ScriptLanguage language;
	String tempPath;
	String generatedCode;
	ScriptTarget(ScriptLanguage language, const ReadableString &tempPath)
	: language(language), tempPath(tempPath) {}
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
	List<Dependency> dependencies;
	ProjectContext() {}
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
void evaluateScript(ScriptTarget &output, ProjectContext &context, Machine &target, const ReadableString &scriptPath);

// Build anything in projectPath.
void build(ScriptTarget &output, const ReadableString &projectPath, Machine &settings);

// Build the project in projectFilePath.
// Settings must be taken by value to prevent side-effects from spilling over between different scripts.
void buildProject(ScriptTarget &output, const ReadableString &projectFilePath, Machine settings);

// Build all projects in projectFolderPath.
void buildProjects(ScriptTarget &output, const ReadableString &projectFolderPath, Machine &settings);
void argumentsToSettings(Machine &settings, const List<String> &arguments, int64_t firstArgument);

#endif
