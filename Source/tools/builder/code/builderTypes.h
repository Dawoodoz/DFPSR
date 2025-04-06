
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
	// Name of this project.
	String projectName;
	// Variables that can be assigned and used for logic.
	List<Flag> variables;
	// The flags to give the compiler.
	List<String> compilerFlags;
	// The flags to give the linker.
	List<String> linkerFlags;
	// The frameworks to give the linker.
	List<String> frameworks;
	// A list of implementation files to start crawling from, usually main.cpp or a disconnected backend implementation.
	List<String> crawlOrigins;
	// Paths to look for other projects in.
	List<String> otherProjectPaths;
	List<Machine> otherProjectSettings;
	// Filenames to create projects for automatically without needing project files for each.
	//   Useful for running automated tests, so that memory leaks can easily be narrowed down to the test causing the leak.
	List<String> projectFromSourceFilenames;
	List<Machine> projectFromSourceSettings;
	// When activeStackDepth < currentStackDepth, we are skipping false cases.
	int64_t currentStackDepth = 0; // How many scopes we are inside of, from the root script including all the others.
	int64_t activeStackDepth = 0;
	Machine(const ReadableString &projectName) : projectName(projectName) {}
};

enum class Extension {
	Unknown,
	H,   // C/C++ header
	Hpp, // C++ header
	C,   // C
	Cpp, // C++
	M,   // Objective-C
	Mm   // Objective-C++
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
	uint64_t contentChecksum = 0;
	bool visited = false; // Used to avoid infinite loops while traversing dependencies.
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
	String sourcePath, objectPath, compilerName, compileFrom;
	List<String> compilerFlags;
	SourceObject(uint64_t identityChecksum, uint64_t combinedChecksum, const ReadableString& sourcePath, const ReadableString& objectPath, const List<String> &compilerFlags, const ReadableString& compilerName, const ReadableString& compileFrom)
	: identityChecksum(identityChecksum), combinedChecksum(combinedChecksum), sourcePath(sourcePath), objectPath(objectPath), compilerFlags(compilerFlags), compilerName(compilerName), compileFrom(compileFrom) {}
};

struct LinkingStep {
	String compilerName, compileFrom, binaryName;
	List<String> linkerFlags; // Linker flags are given as separate arguments to the linker.
	List<String> frameworks; // Frameworks are like static libraries to link with, but uses -framework as a separate argument to the compiler before the framework's name.
	List<int64_t> sourceObjectIndices;
	bool executeResult;
	LinkingStep(const ReadableString &compilerName, const ReadableString &compileFrom, const ReadableString &binaryName, const List<String> &linkerFlags, const List<String> &frameworks, const List<int64_t> &sourceObjectIndices, bool executeResult)
	: compilerName(compilerName), compileFrom(compileFrom), binaryName(binaryName), linkerFlags(linkerFlags), frameworks(frameworks), sourceObjectIndices(sourceObjectIndices), executeResult(executeResult) {}
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
