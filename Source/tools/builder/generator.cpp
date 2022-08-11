
#include "generator.h"

using namespace dsr;

static uint64_t checksum(const ReadableString& text) {
	uint64_t a = 0x8C2A03D4;
	uint64_t b = 0xF42B1583;
	uint64_t c = 0xA6815E74;
	uint64_t d = 0;
	for (int i = 0; i < string_length(text); i++) {
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
	for (int i = 0; i < buffer_getSize(buffer); i++) {
		a = (b * c + ((i * 3756 + 2654) & 58043)) & 0xFFFFFFFF;
		b = (231 + data[i] * (a & 154) + c * 867 + 28294061) & 0xFFFFFFFF;
		c = (a ^ b ^ (data[i] * 1543217521)) & 0xFFFFFFFF;
		d = d ^ (a << 32) ^ b ^ (c << 16);
	}
	return d;
}

static int64_t findDependency(ProjectContext &context, const ReadableString& findPath);
static void resolveConnection(Connection &connection);
static void resolveDependency(Dependency &dependency);
static String findSourceFile(const ReadableString& headerPath, bool acceptC, bool acceptCpp);
static void flushToken(List<String> &target, String &currentToken);
static void tokenize(List<String> &target, const ReadableString& line);
static void interpretPreprocessing(ProjectContext &context, int64_t parentIndex, const List<String> &tokens, const ReadableString &parentFolder, int64_t lineNumber);
static void analyzeCode(ProjectContext &context, int64_t parentIndex, String content, const ReadableString &parentFolder);

static int64_t findDependency(ProjectContext &context, const ReadableString& findPath) {
	for (int d = 0; d < context.dependencies.length(); d++) {
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
	for (int l = 0; l < dependency.links.length(); l++) {
		resolveConnection(context, dependency.links[l]);
	}
	for (int i = 0; i < dependency.includes.length(); i++) {
		resolveConnection(context, dependency.includes[i]);
	}
}

void resolveDependencies(ProjectContext &context) {
	for (int d = 0; d < context.dependencies.length(); d++) {
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
	for (int i = 0; i < string_length(line); i++) {
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

static void interpretPreprocessing(ProjectContext &context, int64_t parentIndex, const List<String> &tokens, const ReadableString &parentFolder, int64_t lineNumber) {
	if (tokens.length() >= 3) {
		if (string_match(tokens[1], U"include")) {
			if (tokens[2][0] == U'\"') {
				String relativePath = string_unmangleQuote(tokens[2]);
				String absolutePath = file_getTheoreticalAbsolutePath(relativePath, parentFolder, LOCAL_PATH_SYNTAX);
				context.dependencies[parentIndex].includes.pushConstruct(absolutePath, lineNumber);
				analyzeFromFile(context, absolutePath);
			}
		}
	}
}

static void analyzeCode(ProjectContext &context, int64_t parentIndex, String content, const ReadableString &parentFolder) {
	List<String> tokens;
	bool continuingLine = false;
	int64_t lineNumber = 0;
	string_split_callback(content, U'\n', true, [&parentIndex, &parentFolder, &tokens, &continuingLine, &lineNumber, &context](ReadableString line) {
		lineNumber++;
		if (line[0] == U'#' || continuingLine) {
			tokenize(tokens, line);
			// Continuing pre-processing line using \ at the end.
			continuingLine = line[string_length(line) - 1] == U'\\';
		} else {
			continuingLine = false;
		}
		if (!continuingLine && tokens.length() > 0) {
			interpretPreprocessing(context, parentIndex, tokens, parentFolder, lineNumber);
			tokens.clear();
		}
	});
}

void analyzeFromFile(ProjectContext &context, const ReadableString& absolutePath) {
	if (findDependency(context, absolutePath) != -1) {
		// Already analyzed the current entry. Abort to prevent duplicate dependencies.
		return;
	}
	int lastDotIndex = string_findLast(absolutePath, U'.');
	if (lastDotIndex != -1) {
		Extension extension = extensionFromString(string_after(absolutePath, lastDotIndex));
		if (extension != Extension::Unknown) {
			// The old length will be the new dependency's index.
			int64_t parentIndex = context.dependencies.length();
			// Get the file's binary content.
			Buffer fileBuffer = file_loadBuffer(absolutePath);
			// Get the checksum
			uint64_t contentChecksum = checksum(fileBuffer);
			context.dependencies.pushConstruct(absolutePath, extension, contentChecksum);
			if (extension == Extension::H || extension == Extension::Hpp) {
				// The current file is a header, so look for an implementation with the corresponding name.
				String sourcePath = findSourceFile(absolutePath, extension == Extension::H, true);
				// If found:
				if (string_length(sourcePath) > 0) {
					// Remember that anything using the header will have to link with the implementation.
					context.dependencies[parentIndex].links.pushConstruct(sourcePath);
					// Look for included headers in the implementation file.
					analyzeFromFile(context, sourcePath);
				}
			}
			// Interpret the file's content.
			analyzeCode(context, parentIndex, string_loadFromMemory(fileBuffer), file_getRelativeParentFolder(absolutePath));
		}
	}
}

static void debugPrintDependencyList(const List<Connection> &connnections, const ReadableString verb) {
	for (int c = 0; c < connnections.length(); c++) {
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
	for (int d = 0; d < context.dependencies.length(); d++) {
		printText(U"* ", file_getPathlessName(context.dependencies[d].path), U"\n");
		debugPrintDependencyList(context.dependencies[d].includes, U"including");
		debugPrintDependencyList(context.dependencies[d].links, U"linking");
	}
}

static void script_printMessage(String &output, ScriptLanguage language, const ReadableString message) {
	if (language == ScriptLanguage::Batch) {
		string_append(output, U"echo ", message, U"\n");
	} else if (language == ScriptLanguage::Bash) {
		string_append(output, U"echo ", message, U"\n");
	}
}

static void traverserHeaderChecksums(ProjectContext &context, uint64_t &target, int64_t dependencyIndex) {
	// Use checksums from headers
	for (int h = 0; h < context.dependencies[dependencyIndex].includes.length(); h++) {
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
	for (int d = 0; d < context.dependencies.length(); d++) {
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
	for (int i = 0; i < settings.compilerFlags.length(); i++) {
		string_append(generatedCompilerFlags, " ", settings.compilerFlags[i]);
	}
	String linkerFlags;
	for (int i = 0; i < settings.linkerFlags.length(); i++) {
		string_append(linkerFlags, " -l", settings.linkerFlags[i]);
	}
	printText(U"Generating build instructions for ", programPath, U" using settings:\n");
	printText(U"  Compiler flags:", generatedCompilerFlags, U"\n");
	printText(U"  Linker flags:", linkerFlags, U"\n");
	for (int v = 0; v < settings.variables.length(); v++) {
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
	for (int d = 0; d < context.dependencies.length(); d++) {
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

static ScriptLanguage identifyLanguage(const ReadableString &filename) {
	String scriptExtension = string_upperCase(file_getExtension(filename));
	if (string_match(scriptExtension, U"BAT")) {
		return ScriptLanguage::Batch;
	} else if (string_match(scriptExtension, U"SH")) {
		return ScriptLanguage::Bash;
	} else {
		throwError(U"Could not identify the scripting language of ", filename, U". Use *.bat or *.sh.\n");
		return ScriptLanguage::Unknown;
	}
}

void setCompilationFolder(String &generatedCode, ScriptLanguage language, String &currentPath, const ReadableString &newPath) {
	if (!string_match(currentPath, newPath)) {
		if (string_length(currentPath) > 0) {
			if (language == ScriptLanguage::Batch) {
				string_append(generatedCode,  "popd\n");
			} else if (language == ScriptLanguage::Bash) {
				string_append(generatedCode, U")\n");
			}
		}
		if (string_length(newPath) > 0) {
			if (language == ScriptLanguage::Batch) {
				string_append(generatedCode,  "pushd ", newPath, "\n");
			} else if (language == ScriptLanguage::Bash) {
				string_append(generatedCode, U"(cd ", newPath, ";\n");
			}
		}
	}
}

void generateCompilationScript(SessionContext &input, const ReadableString &scriptPath) {
	printText(U"Generating build script\n");
	String generatedCode;
	ScriptLanguage language = identifyLanguage(scriptPath);
	if (language == ScriptLanguage::Batch) {
		string_append(generatedCode, U"@echo off\n\n");
	} else if (language == ScriptLanguage::Bash) {
		string_append(generatedCode, U"#!/bin/bash\n\n");
	}

	// Keep track of the current path, so that it only changes when needed.
	String currentPath;

	// Generate code for compiling source code into objects.
	printText(U"Generating code for compiling ", input.sourceObjects.length(), U" objects.\n");
	for (int o = 0; o < input.sourceObjects.length(); o++) {
		SourceObject *sourceObject = &(input.sourceObjects[o]);
		printText(U"\t* ", sourceObject->sourcePath, U"\n");
		setCompilationFolder(generatedCode, language, currentPath, sourceObject->compileFrom);
		if (language == ScriptLanguage::Batch) {
			string_append(generatedCode,  U"if exist ", sourceObject->objectPath, U" (\n");
		} else if (language == ScriptLanguage::Bash) {
			string_append(generatedCode, U"if [ -e \"", sourceObject->objectPath, U"\" ]; then\n");
		}
		script_printMessage(generatedCode, language, string_combine(U"Reusing ", sourceObject->sourcePath, U" ID:", sourceObject->identityChecksum, U"."));
		if (language == ScriptLanguage::Batch) {
			string_append(generatedCode,  U") else (\n");
		} else if (language == ScriptLanguage::Bash) {
			string_append(generatedCode, U"else\n");
		}
		String compilerFlags = sourceObject->generatedCompilerFlags;
		script_printMessage(generatedCode, language, string_combine(U"Compiling ", sourceObject->sourcePath, U" ID:", sourceObject->identityChecksum, U" with ", compilerFlags, U"."));
		string_append(generatedCode, sourceObject->compilerName, compilerFlags, U" -c ", sourceObject->sourcePath, U" -o ", sourceObject->objectPath, U"\n");
		if (language == ScriptLanguage::Batch) {
			string_append(generatedCode,  ")\n");
		} else if (language == ScriptLanguage::Bash) {
			string_append(generatedCode, U"fi\n");
		}
	}

	// Generate code for linking objects into executables.
	printText(U"Generating code for linking ", input.linkerSteps.length(), U" executables:\n");
	for (int l = 0; l < input.linkerSteps.length(); l++) {
		LinkingStep *linkingStep = &(input.linkerSteps[l]);
		String programPath = linkingStep->binaryName;
		printText(U"\tGenerating code for linking ", programPath, U" of :\n");
		setCompilationFolder(generatedCode, language, currentPath, linkingStep->compileFrom);
		String linkerFlags;
		for (int lib = 0; lib < linkingStep->linkerFlags.length(); lib++) {
			String library = linkingStep->linkerFlags[lib];
			string_append(linkerFlags, " -l", library);
			printText(U"\t\t* ", library, U" library\n");
		}
		// Generate a list of object paths from indices.
		String allObjects;
		for (int i = 0; i < linkingStep->sourceObjectIndices.length(); i++) {
			int64_t objectIndex = linkingStep->sourceObjectIndices[i];
			SourceObject *sourceObject = &(input.sourceObjects[objectIndex]);
			if (objectIndex >= 0 || objectIndex < input.sourceObjects.length()) {
				printText(U"\t\t* ", sourceObject->sourcePath, U"\n");
				string_append(allObjects, U" ", sourceObject->objectPath);
			} else {
				throwError(U"Object index ", objectIndex, U" is out of bound ", 0, U"..", (input.sourceObjects.length() - 1), U"\n");
			}
		}
		// Generate the code for building.
		if (string_length(linkerFlags) > 0) {
			script_printMessage(generatedCode, language, string_combine(U"Linking ", programPath, U" with", linkerFlags, U"."));
		} else {
			script_printMessage(generatedCode, language, string_combine(U"Linking ", programPath, U"."));
		}
		string_append(generatedCode, linkingStep->compilerName, allObjects, linkerFlags, U" -o ", programPath, U"\n");
		if (linkingStep->executeResult) {
			script_printMessage(generatedCode, language, string_combine(U"Starting ", programPath));
			string_append(generatedCode, programPath, U"\n");
			script_printMessage(generatedCode, language, U"The program terminated.");
		}
	}
	setCompilationFolder(generatedCode, language, currentPath, U"");
	script_printMessage(generatedCode, language, U"Done building.");

	// Save the script.
	printText(U"Saving script to ", scriptPath, "\n");
	if (language == ScriptLanguage::Batch) {
		string_save(scriptPath, generatedCode);
	} else if (language == ScriptLanguage::Bash) {
		string_save(scriptPath, generatedCode, CharacterEncoding::BOM_UTF8, LineEncoding::Lf);
	}
}
