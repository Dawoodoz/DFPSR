
#ifndef DSR_BUILDER_TYPES
#define DSR_BUILDER_TYPES

#include "../../../DFPSR/api/stringAPI.h"

using namespace dsr;

struct Flag {
	// Flags created externally using argumentsToSettings from either the command line or another project, will be marked as inherited and given to the next call.
	bool inherited;
	String key, value;
	Flag() {}
	Flag(const ReadableString &key, const ReadableString &value, bool inherited)
	: key(key), value(value), inherited(inherited) {}
};

struct Machine {
	List<Flag> variables;
	List<String> compilerFlags;
	List<String> linkerFlags;
	List<String> crawlOrigins;
	List<String> otherProjectPaths; // Corresponding to otherProjectSettings.
	List<Machine> otherProjectSettings; // Corresponding to otherProjectPaths.
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
	Dependency(const ReadableString& path, Extension extension)
	: path(path), extension(extension) {}
};

struct ProjectContext {
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


#endif
