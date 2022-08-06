
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

struct Connection {
	String path;
	int64_t lineNumber = -1;
	int64_t dependencyIndex = -1;
	Connection(const ReadableString& path)
	: path(path) {}
	Connection(const ReadableString& path, int64_t lineNumber)
	: path(path), lineNumber(lineNumber) {}
};

enum class Extension {
	Unknown, H, Hpp, C, Cpp
};
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
List<Dependency> dependencies;

static int64_t findDependency(const ReadableString& findPath);
static void resolveConnection(Connection &connection);
static void resolveDependency(Dependency &dependency);
static String findSourceFile(const ReadableString& headerPath, bool acceptC, bool acceptCpp);
static void flushToken(List<String> &target, String &currentToken);
static void tokenize(List<String> &target, const ReadableString& line);
static void interpretPreprocessing(int64_t parentIndex, const List<String> &tokens, const ReadableString &parentFolder, int64_t lineNumber);
static void interpretPreprocessing(int64_t parentIndex, const List<String> &tokens, const ReadableString &parentFolder, int64_t lineNumber);
static void analyzeCode(int64_t parentIndex, String content, const ReadableString &parentFolder);

static int64_t findDependency(const ReadableString& findPath) {
	for (int d = 0; d < dependencies.length(); d++) {
		if (string_match(dependencies[d].path, findPath)) {
			return d;
		}
	}
	return -1;
}

static void resolveConnection(Connection &connection) {
	connection.dependencyIndex = findDependency(connection.path);
}

static void resolveDependency(Dependency &dependency) {
	for (int l = 0; l < dependency.links.length(); l++) {
		resolveConnection(dependency.links[l]);
	}
	for (int i = 0; i < dependency.includes.length(); i++) {
		resolveConnection(dependency.includes[i]);
	}
}

void resolveDependencies() {
	for (int d = 0; d < dependencies.length(); d++) {
		resolveDependency(dependencies[d]);
	}
}

static String findSourceFile(const ReadableString& headerPath, bool acceptC, bool acceptCpp) {
	int lastDotIndex = string_findLast(headerPath, U'.');
	if (lastDotIndex != -1) {
		ReadableString extensionlessPath = string_removeOuterWhiteSpace(string_before(headerPath, lastDotIndex));
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

static void interpretPreprocessing(int64_t parentIndex, const List<String> &tokens, const ReadableString &parentFolder, int64_t lineNumber) {
	if (tokens.length() >= 3) {
		if (string_match(tokens[1], U"include")) {
			if (tokens[2][0] == U'\"') {
				String relativePath = string_unmangleQuote(tokens[2]);
				String absolutePath = file_getTheoreticalAbsolutePath(relativePath, parentFolder, LOCAL_PATH_SYNTAX);
				dependencies[parentIndex].includes.pushConstruct(absolutePath, lineNumber);
				analyzeFromFile(absolutePath);
			}
		}
	}
}

static void analyzeCode(int64_t parentIndex, String content, const ReadableString &parentFolder) {
	List<String> tokens;
	bool continuingLine = false;
	int64_t lineNumber = 0;
	string_split_callback(content, U'\n', true, [&parentIndex, &parentFolder, &tokens, &continuingLine, &lineNumber](ReadableString line) {
		lineNumber++;
		if (line[0] == U'#' || continuingLine) {
			tokenize(tokens, line);
			// Continuing pre-processing line using \ at the end.
			continuingLine = line[string_length(line) - 1] == U'\\';
		} else {
			continuingLine = false;
		}
		if (!continuingLine && tokens.length() > 0) {
			interpretPreprocessing(parentIndex, tokens, parentFolder, lineNumber);
			tokens.clear();
		}
	});
}

void analyzeFromFile(const ReadableString& absolutePath) {
	if (findDependency(absolutePath) != -1) {
		// Already analyzed the current entry. Abort to prevent duplicate dependencies.
		return;
	}
	int lastDotIndex = string_findLast(absolutePath, U'.');
	if (lastDotIndex != -1) {
		Extension extension = extensionFromString(string_after(absolutePath, lastDotIndex));
		if (extension != Extension::Unknown) {
			// The old length will be the new dependency's index.
			int64_t parentIndex = dependencies.length();
			// Get the file's binary content.
			Buffer fileBuffer = file_loadBuffer(absolutePath);
			// Get the checksum
			uint64_t contentChecksum = checksum(fileBuffer);
			dependencies.pushConstruct(absolutePath, extension, contentChecksum);
			if (extension == Extension::H || extension == Extension::Hpp) {
				// The current file is a header, so look for an implementation with the corresponding name.
				String sourcePath = findSourceFile(absolutePath, extension == Extension::H, true);
				// If found:
				if (string_length(sourcePath) > 0) {
					// Remember that anything using the header will have to link with the implementation.
					dependencies[parentIndex].links.pushConstruct(sourcePath);
					// Look for included headers in the implementation file.
					analyzeFromFile(sourcePath);
				}
			}
			// Interpret the file's content.
			analyzeCode(parentIndex, string_loadFromMemory(fileBuffer), file_getRelativeParentFolder(absolutePath));
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

void printDependencies() {
	for (int d = 0; d < dependencies.length(); d++) {
		printText(U"* ", file_getPathlessName(dependencies[d].path), U"\n");
		debugPrintDependencyList(dependencies[d].includes, U"including");
		debugPrintDependencyList(dependencies[d].links, U"linking");
	}
}

static ScriptLanguage identifyLanguage(const ReadableString filename) {
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

static void script_printMessage(String &output, ScriptLanguage language, const ReadableString message) {
	if (language == ScriptLanguage::Batch) {
		string_append(output, U"echo ", message, U"\n");
	} else if (language == ScriptLanguage::Bash) {
		string_append(output, U"echo ", message, U"\n");
	}
}

static void script_executeLocalBinary(String &output, ScriptLanguage language, const ReadableString code) {
	if (language == ScriptLanguage::Batch) {
		string_append(output, code, ".exe\n");
	} else if (language == ScriptLanguage::Bash) {
		string_append(output, file_combinePaths(U".", code), U";\n");
	}
}

static void traverserHeaderChecksums(uint64_t &target, int64_t dependencyIndex) {
	// Use checksums from headers
	for (int h = 0; h < dependencies[dependencyIndex].includes.length(); h++) {
		int64_t includedIndex = dependencies[dependencyIndex].includes[h].dependencyIndex;
		if (!dependencies[includedIndex].visited) {
			//printText(U"	traverserHeaderChecksums(", includedIndex, U") ", dependencies[includedIndex].path, "\n");
			// Bitwise exclusive or is both order independent and entropy preserving for non-repeated content.
			target = target ^ dependencies[includedIndex].contentChecksum;
			// Just have to make sure that the same checksum is not used twice.
			dependencies[includedIndex].visited = true;
			// Use checksums from headers recursively
			traverserHeaderChecksums(target, includedIndex);
		}
	}
}

static uint64_t getCombinedChecksum(int64_t dependencyIndex) {
	//printText(U"getCombinedChecksum(", dependencyIndex, U") ", dependencies[dependencyIndex].path, "\n");
	for (int d = 0; d < dependencies.length(); d++) {
		dependencies[d].visited = false;
	}
	dependencies[dependencyIndex].visited = true;
	uint64_t result = dependencies[dependencyIndex].contentChecksum;
	traverserHeaderChecksums(result, dependencyIndex);
	return result;
}

struct SourceObject {
	uint64_t identityChecksum = 0; // Identification number for the object's name.
	uint64_t combinedChecksum = 0; // Combined content of the source file and all included headers recursively.
	String sourcePath, objectPath;
	SourceObject(const ReadableString& sourcePath, const ReadableString& tempFolder, const ReadableString& identity, int64_t dependencyIndex)
	: identityChecksum(checksum(identity)), combinedChecksum(getCombinedChecksum(dependencyIndex)), sourcePath(sourcePath) {
		// By making the content checksum a part of the name, one can switch back to an older version without having to recompile everything again.
		// Just need to clean the temporary folder once in a while because old versions can take a lot of space.
		this->objectPath = file_combinePaths(tempFolder, string_combine(U"dfpsr_", this->identityChecksum, U"_", this->combinedChecksum, U".o"));
	}
};

void generateCompilationScript(const Machine &settings, const ReadableString& projectPath) {
	ReadableString scriptPath = getFlag(settings, U"ScriptPath", U"");
	ReadableString tempFolder = file_getAbsoluteParentFolder(scriptPath);
	if (string_length(scriptPath) == 0) {
		printText(U"No script path was given, skipping script generation\n");
		return;
	}
	ScriptLanguage language = identifyLanguage(scriptPath);
	scriptPath = file_getTheoreticalAbsolutePath(scriptPath, projectPath);
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

	// Convert lists of linker and compiler flags into strings.
	// TODO: Give a warning if two contradictory flags are used, such as optimization levels and language versions.
	// TODO: Make sure that no spaces are inside of the flags, because that can mess up detection of pre-existing and contradictory arguments.
	String compilerFlags;
	for (int i = 0; i < settings.compilerFlags.length(); i++) {
		string_append(compilerFlags, " ", settings.compilerFlags[i]);
	}
	String linkerFlags;
	for (int i = 0; i < settings.linkerFlags.length(); i++) {
		string_append(linkerFlags, " -l", settings.linkerFlags[i]);
	}

	// Interpret ProgramPath relative to the project path.
	ReadableString binaryPath = getFlag(settings, U"ProgramPath", language == ScriptLanguage::Batch ? U"program.exe" : U"program");
	binaryPath = file_getTheoreticalAbsolutePath(binaryPath, projectPath);

	String output;
	if (language == ScriptLanguage::Batch) {
		string_append(output, U"@echo off\n\n");
	} else if (language == ScriptLanguage::Bash) {
		string_append(output, U"#!/bin/bash\n\n");
	} else {
		printText(U"The type of script could not be identified for ", scriptPath, U"!\nUse *.bat for Batch or *.sh for Bash.\n");
		return;
	}
	List<SourceObject> sourceObjects;
	bool hasSourceCode = false;
	bool needCppCompiler = false;
	for (int d = 0; d < dependencies.length(); d++) {
		Extension extension = dependencies[d].extension;
		if (extension == Extension::Cpp) {
			needCppCompiler = true;
		}
		if (extension == Extension::C || extension == Extension::Cpp) {
			// Dependency paths are already absolute from the recursive search.
			String sourcePath = dependencies[d].path;
			String identity = string_combine(sourcePath, compilerFlags, projectPath);
			sourceObjects.pushConstruct(sourcePath, tempFolder, identity, d);
			if (file_getEntryType(sourcePath) != EntryType::File) {
				throwError(U"The source file ", sourcePath, U" could not be found!\n");
			} else {
				hasSourceCode = true;
			}
		}
	}
	if (hasSourceCode) {
		// TODO: Give a warning if a known C compiler incapable of handling C++ is given C++ source code when needCppCompiler is true.
		if (changePath) {
			// Go into the requested folder.
			if (language == ScriptLanguage::Batch) {
				string_append(output,  "pushd ", compileFrom, "\n");
			} else if (language == ScriptLanguage::Bash) {
				string_append(output, U"(cd ", compileFrom, ";\n");
			}
		}
		String allObjects;
		for (int i = 0; i < sourceObjects.length(); i++) {
			if (language == ScriptLanguage::Batch) {
				string_append(output,  U"if exist ", sourceObjects[i].objectPath, U" (\n");
			} else if (language == ScriptLanguage::Bash) {
				string_append(output, U"if [ -e \"", sourceObjects[i].objectPath, U"\" ]; then\n");
			}
			script_printMessage(output, language, string_combine(U"Reusing ", sourceObjects[i].sourcePath, U" ID:", sourceObjects[i].identityChecksum, U"."));
			if (language == ScriptLanguage::Batch) {
				string_append(output,  U") else (\n");
			} else if (language == ScriptLanguage::Bash) {
				string_append(output, U"else\n");
			}
			script_printMessage(output, language, string_combine(U"Compiling ", sourceObjects[i].sourcePath, U" ID:", sourceObjects[i].identityChecksum, U" with ", compilerFlags, U"."));
			string_append(output, compilerName, compilerFlags, U" -c ", sourceObjects[i].sourcePath, U" -o ", sourceObjects[i].objectPath, U"\n");
			if (language == ScriptLanguage::Batch) {
				string_append(output,  ")\n");
			} else if (language == ScriptLanguage::Bash) {
				string_append(output, U"fi\n");
			}
			// Remember each object name for linking.
			string_append(allObjects, U" ", sourceObjects[i].objectPath);
		}
		script_printMessage(output, language, string_combine(U"Linking with ", linkerFlags, U"."));
		string_append(output, compilerName, allObjects, linkerFlags, U" -o ", binaryPath, U"\n");
		if (changePath) {
			// Get back to the previous folder.
			if (language == ScriptLanguage::Batch) {
				string_append(output,  "popd\n");
			} else if (language == ScriptLanguage::Bash) {
				string_append(output, U")\n");
			}
		}
		script_printMessage(output, language, U"Done compiling.");
		script_printMessage(output, language, string_combine(U"Starting ", binaryPath));
		script_executeLocalBinary(output, language, binaryPath);
		script_printMessage(output, language, U"The program terminated.");
		if (language == ScriptLanguage::Batch) {
			// Windows might close the window before you have time to read the results or error messages of a CLI application, so pause at the end.
			string_append(output, U"pause\n");
		}
		if (language == ScriptLanguage::Batch) {
			string_save(scriptPath, output);
		} else if (language == ScriptLanguage::Bash) {
			string_save(scriptPath, output, CharacterEncoding::BOM_UTF8, LineEncoding::Lf);
		}
	} else {
		printText("Filed to find any source code to compile.\n");
	}
}
