
#include "generator.h"
#include "Machine.h"

using namespace dsr;

static Extension extensionFromString(const ReadableString& extensionName) {
	String upperName = string_upperCase(string_removeOuterWhiteSpace(extensionName));
	Extension result = Extension::Unknown;
	if (string_match(upperName, U"H")) {
		result = Extension::H;
	} else if (string_match(upperName, U"HPP")) {
		result = Extension::Hpp;
	} else if (string_match(upperName, U"C")) {
		result = Extension::C;
	} else if (string_match(upperName, U"CPP")) {
		result = Extension::Cpp;
	}
	return result;
}

static uint64_t checksum(const ReadableString& text) {
	uint64_t a = 0x8C2A03D4;
	uint64_t b = 0xF42B1583;
	uint64_t c = 0xA6815E74;
	uint64_t d = 0;
	for (int64_t i = 0; i < string_length(text); i++) {
		a = (b * c + ((i * 3756 + 2654) & 58043)) & 0xFFFFFFFF;
		b = (231 + text[i] * (a & 154) + c * 867 + 28294061) & 0xFFFFFFFF;
		c = (a ^ b ^ (text[i] * 1543217521)) & 0xFFFFFFFF;
		d = d ^ (a << 32) ^ b ^ (c << 16);
	}
	return d;
}

static uint64_t checksum(const Buffer& buffer) {
	SafePointer<uint8_t> data = buffer_getSafeData<uint8_t>(buffer, "checksum input buffer");
	uint64_t a = 0x8C2A03D4;
	uint64_t b = 0xF42B1583;
	uint64_t c = 0xA6815E74;
	uint64_t d = 0;
	for (int64_t i = 0; i < buffer_getSize(buffer); i++) {
		a = (b * c + ((i * 3756 + 2654) & 58043)) & 0xFFFFFFFF;
		b = (231 + data[i] * (a & 154) + c * 867 + 28294061) & 0xFFFFFFFF;
		c = (a ^ b ^ (data[i] * 1543217521)) & 0xFFFFFFFF;
		d = d ^ (a << 32) ^ b ^ (c << 16);
	}
	return d;
}

static int64_t findDependency(ProjectContext &context, const ReadableString& findPath) {
	for (int64_t d = 0; d < context.dependencies.length(); d++) {
		if (string_match(context.dependencies[d].path, findPath)) {
			return d;
		}
	}
	return -1;
}

static void resolveConnection(ProjectContext &context, Connection &connection) {
	connection.dependencyIndex = findDependency(context, connection.path);
}

static void resolveDependency(ProjectContext &context, Dependency &dependency) {
	for (int64_t l = 0; l < dependency.links.length(); l++) {
		resolveConnection(context, dependency.links[l]);
	}
	for (int64_t i = 0; i < dependency.includes.length(); i++) {
		resolveConnection(context, dependency.includes[i]);
	}
}

void resolveDependencies(ProjectContext &context) {
	for (int64_t d = 0; d < context.dependencies.length(); d++) {
		resolveDependency(context, context.dependencies[d]);
	}
}

static String findSourceFile(const ReadableString& headerPath, bool acceptC, bool acceptCpp) {
	if (file_hasExtension(headerPath)) {
		ReadableString extensionlessPath = file_getExtensionless(headerPath);
		String cPath = extensionlessPath + U".c";
		String cppPath = extensionlessPath + U".cpp";
		if (acceptC && file_getEntryType(cPath) == EntryType::File) {
			return cPath;
		} else if (acceptCpp && file_getEntryType(cppPath) == EntryType::File) {
			return cppPath;
		}
	}
	return U"";
}

static void flushToken(List<String> &target, String &currentToken) {
	if (string_length(currentToken) > 0) {
		target.push(currentToken);
		currentToken = U"";
	}
}

static void tokenize(List<String> &target, const ReadableString& line) {
	String currentToken;
	for (int64_t i = 0; i < string_length(line); i++) {
		DsrChar c = line[i];
		DsrChar nextC = line[i + 1];
		if (c == U'#' && nextC == U'#') {
			// Appending tokens using ##
			i++;
		} else if (c == U'#' || c == U'(' || c == U')' || c == U'[' || c == U']' || c == U'{' || c == U'}') {
			// Atomic token of a single character
			flushToken(target, currentToken);
			string_appendChar(currentToken, c);
			flushToken(target, currentToken);
		} else if (c == U' ' || c == U'\t') {
			// Whitespace
			flushToken(target, currentToken);
		} else {
			string_appendChar(currentToken, c);
		}
	}
	flushToken(target, currentToken);
}

// When CACHED_ANALYSIS is enabled, files will only be analyzed once per session, by remembering them from previous projects.
//   If features that require a different type of analysis per project are implemented, this can easily be turned off.
#define CACHED_ANALYSIS

#ifdef CACHED_ANALYSIS
	// Remembering previous results from analyzing the same files.
	List<Dependency> analysisCache;
#endif

void analyzeFile(Dependency &result, const ReadableString& absolutePath, Extension extension) {
	#ifdef CACHED_ANALYSIS
		// Check if the file has already been analyzed.
		for (int c = 0; c < analysisCache.length(); c++) {
			if (string_match(analysisCache[c].path, absolutePath)) {
				// Clone all the results to keep projects separate in memory for safety.
				result = analysisCache[c];
				return;
			}
		}
	#endif
	// Get the file's binary content.
	Buffer fileBuffer = file_loadBuffer(absolutePath);
	// Get the checksum
	result.contentChecksum = checksum(fileBuffer);
	if (extension == Extension::H || extension == Extension::Hpp) {
		// The current file is a header, so look for an implementation with the corresponding name.
		String sourcePath = findSourceFile(absolutePath, extension == Extension::H, true);
		// If found:
		if (string_length(sourcePath) > 0) {
			// Remember that anything using the header will have to link with the implementation.
			result.links.pushConstruct(sourcePath);
		}
	}
	// Interpret the file's content.
	String sourceCode = string_loadFromMemory(fileBuffer);
	String parentFolder = file_getRelativeParentFolder(absolutePath);
	List<String> tokens;
	bool continuingLine = false;
	int64_t lineNumber = 0;
	string_split_callback(sourceCode, U'\n', true, [&result, &parentFolder, &tokens, &continuingLine, &lineNumber](ReadableString line) {
		lineNumber++;
		if (line[0] == U'#' || continuingLine) {
			tokenize(tokens, line);
			// Continuing pre-processing line using \ at the end.
			continuingLine = line[string_length(line) - 1] == U'\\';
		} else {
			continuingLine = false;
		}
		if (!continuingLine && tokens.length() > 0) {
			if (tokens.length() >= 3) {
				if (string_match(tokens[1], U"include")) {
					if (tokens[2][0] == U'\"') {
						String relativePath = string_unmangleQuote(tokens[2]);
						String absolutePath = file_getTheoreticalAbsolutePath(relativePath, parentFolder, LOCAL_PATH_SYNTAX);
						result.includes.pushConstruct(absolutePath, lineNumber);
					}
				}
			}
			tokens.clear();
		}
	});
}

void analyzeFromFile(ProjectContext &context, const ReadableString& absolutePath) {
	if (findDependency(context, absolutePath) != -1) {
		// Already analyzed the current entry. Abort to prevent duplicate dependencies.
		return;
	}
	Extension extension = extensionFromString(file_getExtension(absolutePath));
	if (extension != Extension::Unknown) {
		// Create a new dependency for the file.
		int64_t parentIndex = context.dependencies.length();
		context.dependencies.push(Dependency(absolutePath, extension));
		// Summarize the file's content.
		analyzeFile(context.dependencies[parentIndex], absolutePath, extension);
		// Continue analyzing recursively into the file's dependencies.
		for (int64_t i = 0; i < context.dependencies[parentIndex].includes.length(); i++) {
			analyzeFromFile(context, context.dependencies[parentIndex].includes[i].path);
		}
		for (int64_t l = 0; l < context.dependencies[parentIndex].links.length(); l++) {
			analyzeFromFile(context, context.dependencies[parentIndex].links[l].path);
		}
	}
}

static void debugPrintDependencyList(const List<Connection> &connnections, const ReadableString verb) {
	for (int64_t c = 0; c < connnections.length(); c++) {
		int64_t lineNumber = connnections[c].lineNumber;
		if (lineNumber != -1) {
			printText(U"  @", lineNumber, U"\t");
		} else {
			printText(U"    \t");
		}
		printText(U" ", verb, U" ", file_getPathlessName(connnections[c].path), U"\n");
	}
}

void printDependencies(ProjectContext &context) {
	for (int64_t d = 0; d < context.dependencies.length(); d++) {
		printText(U"* ", file_getPathlessName(context.dependencies[d].path), U"\n");
		debugPrintDependencyList(context.dependencies[d].includes, U"including");
		debugPrintDependencyList(context.dependencies[d].links, U"linking");
	}
}

static void traverserHeaderChecksums(ProjectContext &context, uint64_t &target, int64_t dependencyIndex) {
	// Use checksums from headers
	for (int64_t h = 0; h < context.dependencies[dependencyIndex].includes.length(); h++) {
		int64_t includedIndex = context.dependencies[dependencyIndex].includes[h].dependencyIndex;
		if (!context.dependencies[includedIndex].visited) {
			//printText(U"	traverserHeaderChecksums(context, ", includedIndex, U") ", context.dependencies[includedIndex].path, "\n");
			// Bitwise exclusive or is both order independent and entropy preserving for non-repeated content.
			target = target ^ context.dependencies[includedIndex].contentChecksum;
			// Just have to make sure that the same checksum is not used twice.
			context.dependencies[includedIndex].visited = true;
			// Use checksums from headers recursively
			traverserHeaderChecksums(context, target, includedIndex);
		}
	}
}

static uint64_t getCombinedChecksum(ProjectContext &context, int64_t dependencyIndex) {
	//printText(U"getCombinedChecksum(context, ", dependencyIndex, U") ", context.dependencies[dependencyIndex].path, "\n");
	for (int64_t d = 0; d < context.dependencies.length(); d++) {
		context.dependencies[d].visited = false;
	}
	context.dependencies[dependencyIndex].visited = true;
	uint64_t result = context.dependencies[dependencyIndex].contentChecksum;
	traverserHeaderChecksums(context, result, dependencyIndex);
	return result;
}

static int64_t findObject(SessionContext &source, uint64_t identityChecksum) {
	for (int64_t o = 0; o < source.sourceObjects.length(); o++) {
		if (source.sourceObjects[o].identityChecksum == identityChecksum) {
			return o;
		}
	}
	return -1;
}

void gatherBuildInstructions(SessionContext &output, ProjectContext &context, Machine &settings, ReadableString programPath) {
	// The compiler is often a global alias, so the user must supply either an alias or an absolute path.
	ReadableString compilerName = getFlag(settings, U"Compiler", U"g++"); // Assume g++ as the compiler if not specified.
	ReadableString compileFrom = getFlag(settings, U"CompileFrom", U"");
	// Check if the build system was asked to run the compiler from a specific folder.
	bool changePath = (string_length(compileFrom) > 0);
	if (changePath) {
		printText(U"Using ", compilerName, " as the compiler executed from ", compileFrom, ".\n");
	} else {
		printText(U"Using ", compilerName, " as the compiler from the current directory.\n");
	}
	// TODO: Warn if -DNDEBUG, -DDEBUG, or optimization levels are given directly.
	//       Using the variables instead is both more flexible by accepting input arguments
	//       and keeping the same format to better reuse compiled objects.
	if (getFlagAsInteger(settings, U"Debug")) {
		printText(U"Building with debug mode.\n");
		settings.compilerFlags.push(U"-DDEBUG");
	} else {
		printText(U"Building with release mode.\n");
		settings.compilerFlags.push(U"-DNDEBUG");
	}
	if (getFlagAsInteger(settings, U"StaticRuntime")) {
		if (getFlagAsInteger(settings, U"Windows")) {
			printText(U"Building with static runtime. Your application's binary will be bigger but can run without needing any installer.\n");
			settings.compilerFlags.push(U"-static");
			settings.compilerFlags.push(U"-static-libgcc");
			settings.compilerFlags.push(U"-static-libstdc++");
			settings.linkerFlags.push(U"-static");
			settings.linkerFlags.push(U"-static-libgcc");
			settings.linkerFlags.push(U"-static-libstdc++");
		} else {
			printText(U"The target platform does not support static linking of runtime. But don't worry about bundling any runtimes, because it comes with most of the Posix compliant operating systems.\n");
		}
	} else {
		printText(U"Building with dynamic runtime. Don't forget to bundle the C and C++ runtimes for systems that don't have it pre-installed.\n");
	}
	ReadableString optimizationLevel = getFlag(settings, U"Optimization", U"2");
		printText(U"Building with optimization level ", optimizationLevel, U".\n");
	settings.compilerFlags.push(string_combine(U"-O", optimizationLevel));

	// Convert lists of linker and compiler flags into strings.
	// TODO: Give a warning if two contradictory flags are used, such as optimization levels and language versions.
	// TODO: Make sure that no spaces are inside of the flags, because that can mess up detection of pre-existing and contradictory arguments.
	// TODO: Use groups of compiler flags, so that they can be generated in the last step.
	//       This would allow calling the compiler directly when given a folder path for temporary files instead of a script path.
	String generatedCompilerFlags;
	for (int64_t i = 0; i < settings.compilerFlags.length(); i++) {
		string_append(generatedCompilerFlags, " ", settings.compilerFlags[i]);
	}
	String linkerFlags;
	for (int64_t i = 0; i < settings.linkerFlags.length(); i++) {
		string_append(linkerFlags, " -l", settings.linkerFlags[i]);
	}
	printText(U"Generating build instructions for ", programPath, U" using settings:\n");
	printText(U"  Compiler flags:", generatedCompilerFlags, U"\n");
	printText(U"  Linker flags:", linkerFlags, U"\n");
	for (int64_t v = 0; v < settings.variables.length(); v++) {
		printText(U"  * ", settings.variables[v].key, U" = ", settings.variables[v].value);
		if (settings.variables[v].inherited) {
			printText(U" (inherited input)");
		}
		printText(U"\n");
	}
	printText(U"Listing source files to compile in the current session.\n");
	// The current project's global indices to objects shared between all projects being built during the session.
	List<int64_t> sourceObjectIndices;
	bool hasSourceCode = false;
	for (int64_t d = 0; d < context.dependencies.length(); d++) {
		Extension extension = context.dependencies[d].extension;
		if (extension == Extension::C || extension == Extension::Cpp) {
			// Dependency paths are already absolute from the recursive search.
			String sourcePath = context.dependencies[d].path;
			String identity = string_combine(sourcePath, generatedCompilerFlags);
			uint64_t identityChecksum = checksum(identity);
			int64_t previousIndex = findObject(output, identityChecksum);
			if (previousIndex == -1) {
				// Content checksums were created while scanning for source code, so now we just combine each source file's content checksum with all its headers to get the combined checksum.
				// The combined checksum represents the state after all headers are included recursively and given as input for compilation unit generating an object.
				uint64_t combinedChecksum = getCombinedChecksum(context, d);
				String objectPath = file_combinePaths(output.tempPath, string_combine(U"dfpsr_", identityChecksum, U"_", combinedChecksum, U".o"));
				sourceObjectIndices.push(output.sourceObjects.length());
				output.sourceObjects.pushConstruct(identityChecksum, combinedChecksum, sourcePath, objectPath, generatedCompilerFlags, compilerName, compileFrom);
			} else {
				// Link to this pre-existing source file.
				sourceObjectIndices.push(previousIndex);
			}
			hasSourceCode = true;
		}
	}
	if (hasSourceCode) {
		printText(U"Listing target executable ", programPath, " in the current session.\n");
		bool executeResult = getFlagAsInteger(settings, U"Supressed") == 0;
		output.linkerSteps.pushConstruct(compilerName, compileFrom, programPath, settings.linkerFlags, sourceObjectIndices, executeResult);
	} else {
		printText(U"Filed to find any source code to compile when building ", programPath, U".\n");
	}
}

static void crawlSource(ProjectContext &context, const dsr::ReadableString &absolutePath) {
	EntryType pathType = file_getEntryType(absolutePath);
	if (pathType == EntryType::File) {
		printText(U"Crawling for source from ", absolutePath, U".\n");
		analyzeFromFile(context, absolutePath);
	} else if (pathType == EntryType::Folder) {
		printText(U"Crawling was given the folder ", absolutePath, U" but a source file was expected!\n");
	} else if (pathType == EntryType::SymbolicLink) {
		// Symbolic links can point to both files and folder, so we need to follow it and find out what it really is.
		crawlSource(context, file_followSymbolicLink(absolutePath));
	}
}

void build(SessionContext &output, const ReadableString &projectPath, Machine &settings);

static List<String> initializedProjects;
// Using a project file path and input arguments.
void buildProject(SessionContext &output, const ReadableString &projectFilePath, Machine settings) {
	printText("Building project at ", projectFilePath, "\n");
	// Check if this project has begun building previously during this session.
	String absolutePath = file_getAbsolutePath(projectFilePath);
	for (int64_t p = 0; p < initializedProjects.length(); p++) {
		if (string_caseInsensitiveMatch(absolutePath, initializedProjects[p])) {
			throwError(U"Found duplicate requests to build from the same initial script ", absolutePath, U" which could cause non-determinism if different arguments are given to each!\n");
			return;
		}
	}
	// Remember that building of this project has started.
	initializedProjects.push(absolutePath);
	// Evaluate compiler settings while searching for source code mentioned in the project and imported headers.
	printText(U"Executing project file from ", projectFilePath, U".\n");
	ProjectContext context;
	evaluateScript(output, settings, projectFilePath);
	// Find out where things are located.
	String projectPath = file_getAbsoluteParentFolder(projectFilePath);
	// Get the project's name.
	String projectName = file_getPathlessName(file_getExtensionless(projectFilePath));
	// If no application path is given, the new executable will be named after the project and placed in the same folder.
	String fullProgramPath = getFlag(settings, U"ProgramPath", projectName);
	if (string_length(output.executableExtension) > 0) {
		string_append(fullProgramPath, output.executableExtension);
	}
	// Interpret ProgramPath relative to the project path.
	fullProgramPath = file_getTheoreticalAbsolutePath(fullProgramPath, projectPath);
	// Build other projects.
	for (int64_t b = 0; b < settings.otherProjectPaths.length(); b++) {
		build(output, settings.otherProjectPaths[b], settings.otherProjectSettings[b]);
	}
	// If the SkipIfBinaryExists flag is given, we will abort as soon as we have handled its external BuildProjects requests and confirmed that the application exists.
	if (getFlagAsInteger(settings, U"SkipIfBinaryExists") && file_getEntryType(fullProgramPath) == EntryType::File) {
		// SkipIfBinaryExists was active and the binary exists, so abort here to avoid redundant work.
		printText(U"Skipping build of ", projectFilePath, U" because the SkipIfBinaryExists flag was given and ", fullProgramPath, U" was found.\n");
		return;
	}
	// Once we know where the binary is and that it should be built, we can start searching for source code.
	for (int64_t o = 0; o < settings.crawlOrigins.length(); o++) {
		crawlSource(context, settings.crawlOrigins[o]);
	}
	// Once we are done finding all source files, we can resolve the dependencies to create a graph connected by indices.
	resolveDependencies(context);
	if (getFlagAsInteger(settings, U"ListDependencies")) {
		printDependencies(context);
	}
	gatherBuildInstructions(output, context, settings, fullProgramPath);
}

// Using a folder path and input arguments for all projects.
void buildProjects(SessionContext &output, const ReadableString &projectFolderPath, Machine &settings) {
	printText("Building all projects in ", projectFolderPath, "\n");
	file_getFolderContent(projectFolderPath, [&settings, &output](const ReadableString& entryPath, const ReadableString& entryName, EntryType entryType) {
		if (entryType == EntryType::Folder) {
			buildProjects(output, entryPath, settings);
		} else if (entryType == EntryType::File) {
			ReadableString extension = string_upperCase(file_getExtension(entryName));
			if (string_match(extension, U"DSRPROJ")) {
				buildProject(output, entryPath, settings);
			}
		}
	});
}

void build(SessionContext &output, const ReadableString &projectPath, Machine &settings) {
	EntryType entryType = file_getEntryType(projectPath);
	printText("Building anything at ", projectPath, " which is ", entryType, "\n");
	if (entryType == EntryType::File) {
		String extension = string_upperCase(file_getExtension(projectPath));
		if (!string_match(extension, U"DSRPROJ")) {
			printText(U"Can't use the Build keyword with a file that is not a project!\n");
		} else {
			// Build the given project
			buildProject(output, projectPath, settings);
		}
	} else if (entryType == EntryType::Folder) {
		buildProjects(output, projectPath, settings);
	}
}
