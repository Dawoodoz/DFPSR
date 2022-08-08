
#include "Machine.h"
#include "generator.h"
#include "expression.h"
#include "../../DFPSR/api/fileAPI.h"

using namespace dsr;

#define STRING_EXPR(FIRST_TOKEN, LAST_TOKEN) evaluateExpression(target, tokens, FIRST_TOKEN, LAST_TOKEN)
#define INTEGER_EXPR(FIRST_TOKEN, LAST_TOKEN) expression_interpretAsInteger(STRING_EXPR(FIRST_TOKEN, LAST_TOKEN))
#define PATH_EXPR(FIRST_TOKEN, LAST_TOKEN) file_getTheoreticalAbsolutePath(STRING_EXPR(FIRST_TOKEN, LAST_TOKEN), fromPath)

Extension extensionFromString(const ReadableString& extensionName) {
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

int64_t findFlag(const Machine &target, const dsr::ReadableString &key) {
	for (int64_t f = 0; f < target.variables.length(); f++) {
		if (string_caseInsensitiveMatch(key, target.variables[f].key)) {
			return f;
		}
	}
	return -1;
}

ReadableString getFlag(const Machine &target, const dsr::ReadableString &key, const dsr::ReadableString &defaultValue) {
	int64_t existingIndex = findFlag(target, key);
	if (existingIndex == -1) {
		return defaultValue;
	} else {
		return target.variables[existingIndex].value;
	}
}

int64_t getFlagAsInteger(const Machine &target, const dsr::ReadableString &key, int64_t defaultValue) {
	int64_t existingIndex = findFlag(target, key);
	if (existingIndex == -1) {
		return defaultValue;
	} else {
		return string_toInteger(target.variables[existingIndex].value);
	}
}

void assignValue(Machine &target, const dsr::ReadableString &key, const dsr::ReadableString &value, bool inherited) {
	int64_t existingIndex = findFlag(target, key);
	if (existingIndex == -1) {
		target.variables.pushConstruct(string_upperCase(key), expression_unwrapIfNeeded(value), inherited);
	} else {
		target.variables[existingIndex].value = expression_unwrapIfNeeded(value);
		if (inherited) {
			target.variables[existingIndex].inherited = true;
		}
	}
}

static void flushToken(List<String> &targetTokens, String &currentToken) {
	if (string_length(currentToken) > 0) {
		targetTokens.push(currentToken);
		currentToken = U"";
	}
}

static String evaluateExpression(Machine &target, List<String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex) {
	return expression_evaluate(tokens, startTokenIndex, endTokenIndex, [&target](ReadableString identifier) -> String {
		return getFlag(target, identifier, U"");
	});
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

// Copy inherited variables from parent to child.
static void inheritMachine(Machine &child, const Machine &parent) {
	for (int v = 0; v < parent.variables.length(); v++) {
		String key = string_upperCase(parent.variables[v].key);
		if (parent.variables[v].inherited) {
			child.variables.push(parent.variables[v]);
		}
	}
}

static void interpretLine(ScriptTarget &output, ProjectContext &context, Machine &target, List<String> &tokens, const dsr::ReadableString &fromPath) {
	if (tokens.length() > 0) {
		bool activeLine = target.activeStackDepth >= target.currentStackDepth;
		/*
		printText(activeLine ? U"interpret:" : U"ignore:");
		for (int t = 0; t < tokens.length(); t++) {
			printText(U" [", tokens[t], U"]");
		}
		printText(U"\n");
		*/
		ReadableString first = expression_getToken(tokens, 0);
		ReadableString second = expression_getToken(tokens, 1);
		if (activeLine) {
			// TODO: Implement elseif and else cases using a list as a virtual stack,
			//       to remember at which layer the else cases have already been consumed by a true evaluation.
			// TODO: Remember at which depth the script entered, so that importing something can't leave the rest inside of a dangling if or else by accident.
			if (string_caseInsensitiveMatch(first, U"import")) {
				// Get path relative to importing script's path.
				String importPath = PATH_EXPR(1, tokens.length() - 1);
				evaluateScript(output, context, target, importPath);
				if (tokens.length() > 2) { printText(U"Unused tokens after import!\n");}
			} else if (string_caseInsensitiveMatch(first, U"if")) {
				// Being if statement
				bool active = INTEGER_EXPR(1, tokens.length() - 1);
				if (active) {
					target.activeStackDepth++;
				}
				target.currentStackDepth++;
			} else if (string_caseInsensitiveMatch(first, U"end") && string_caseInsensitiveMatch(second, U"if")) {
				// End if statement
				target.currentStackDepth--;
				target.activeStackDepth = target.currentStackDepth;
			} else if (string_caseInsensitiveMatch(first, U"crawl")) {
				// The right hand expression is evaluated into a path relative to the build script and used as the root for searching for source code.
				crawlSource(context, PATH_EXPR(1, tokens.length() - 1));
			} else if (string_caseInsensitiveMatch(first, U"build")) {
				// Build one or more other projects from a project file or folder path, as dependencies.
				//   Having the same external project built twice during the same session is not allowed.
				Machine childTarget;
				inheritMachine(childTarget, target);
				String projectPath = file_getTheoreticalAbsolutePath(expression_unwrapIfNeeded(second), fromPath); // Use the second token as the folder path.
				argumentsToSettings(childTarget, tokens, 2); // Send all tokens after the second token as input arguments to buildProjects.
				printText("Building ", second, " from ", fromPath, " which is ", projectPath, "\n");
				build(output, projectPath, childTarget);
			} else if (string_caseInsensitiveMatch(first, U"link")) {
				// Only the path name itself is needed, so any redundant -l prefixes will be stripped away.
				String libraryName = STRING_EXPR(1, tokens.length() - 1);
				if (libraryName[0] == U'-' && (libraryName[1] == U'l' || libraryName[1] == U'L')) {
					libraryName = string_after(libraryName, 2);
				}
				target.linkerFlags.push(libraryName);
			} else if (string_caseInsensitiveMatch(first, U"compilerflag")) {
				target.compilerFlags.push(STRING_EXPR(1, tokens.length() - 1));
			} else if (string_caseInsensitiveMatch(first, U"message")) {
				// Print a message while evaluating the build script.
				//   This is not done while actually compiling, so it will not know if compilation and linking worked or not.
				printText(STRING_EXPR(1, tokens.length() - 1));
			} else {
				if (tokens.length() == 1) {
					// Mentioning an identifier without assigning anything will assign it to one as a boolean flag.
					assignValue(target, first, U"1", false);
				} else if (string_match(second, U"=")) {
					// TODO: Create in-place math and string operations with different types of assignments.
					//       Maybe use a different syntax beginning with a keyword?
					// TODO: Look for the assignment operator dynamically if references to collection elements are allowed as l-value expressions.
					// Using an equality sign replaces any previous value of the variable.
					assignValue(target, first, STRING_EXPR(2, tokens.length() - 1), false);
				} else {
					// TODO: Give better error messages.
					printText(U"  Ignored unrecognized statement!\n");
				}
			}
		} else {
			if (string_caseInsensitiveMatch(first, U"if")) {
				target.currentStackDepth++;
			} else if (string_caseInsensitiveMatch(first, U"end") && string_caseInsensitiveMatch(second, U"if")) {
				target.currentStackDepth--;
			}
		}
	}
	tokens.clear();
}

void evaluateScript(ScriptTarget &output, ProjectContext &context, Machine &target, const ReadableString &scriptPath) {
	if (file_getEntryType(scriptPath) != EntryType::File) {
		printText(U"The script path ", scriptPath, U" does not exist!\n");
	}
	String projectContent = string_load(scriptPath);
	// Each new script being imported will have its own simulated current path for accessing files and such.
	String projectFolderPath = file_getAbsoluteParentFolder(scriptPath);
	String currentToken;
	List<String> currentLine; // Keep it fast and simple by only remembering tokens for the current line.
	bool quoted = false;
	bool commented = false;
	for (int i = 0; i <= string_length(projectContent); i++) {
		DsrChar c = projectContent[i];
		// The null terminator does not really exist in projectContent,
		//   but dsr::String returns a null character safely when requesting a character out of bound,
		//   which allow interpreting the last line without duplicating code.
		if (c == U'\n' || c == U'\0') {
			// Comment removing everything else.
			flushToken(currentLine, currentToken);
			interpretLine(output, context, target, currentLine, projectFolderPath);
			commented = false; // Automatically end comments at end of line.
			quoted = false; // Automatically end quotes at end of line.
		} else if (c == U'\"') {
			quoted = !quoted;
			string_appendChar(currentToken, c);
		} else if (c == U'#') {
			// Comment removing everything else until a new line comes.
			flushToken(currentLine, currentToken);
			interpretLine(output, context, target, currentLine, projectFolderPath);
			commented = true;
		} else if (!commented) {
			if (quoted) {
				// Insert character into quote.
				string_appendChar(currentToken, c);
			} else {
				if (c == U'(' || c == U')' || c == U'[' || c == U']' || c == U'{' || c == U'}' || c == U'=') {
					// Atomic token of a single character
					flushToken(currentLine, currentToken);
					string_appendChar(currentToken, c);
					flushToken(currentLine, currentToken);
				} else if (c == U' ' || c == U'\t') {
					// Whitespace
					flushToken(currentLine, currentToken);
				} else {
					// Insert unquoted character into token.
					string_appendChar(currentToken, c);
				}
			}
		}
	}
}

static List<String> initializedProjects;
// Using a project file path and input arguments.
void buildProject(ScriptTarget &output, const ReadableString &projectFilePath, Machine settings) {
	printText("Building project at ", projectFilePath, "\n");
	// Check if this project has begun building previously during this session.
	String absolutePath = file_getAbsolutePath(projectFilePath);
	for (int p = 0; p < initializedProjects.length(); p++) {
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
	evaluateScript(output, context, settings, projectFilePath);
	// Find out where things are located.
	String projectPath = file_getAbsoluteParentFolder(projectFilePath);
	// Interpret ProgramPath relative to the project path.
	String fullProgramPath = getFlag(settings, U"ProgramPath", U"program");
	if (output.language == ScriptLanguage::Batch) {
		string_append(fullProgramPath, U".exe");
	}
	fullProgramPath = file_getTheoreticalAbsolutePath(fullProgramPath, projectPath);
	// If the SkipIfBinaryExists flag is given, we will abort as soon as we have handled its external BuildProjects requests and confirmed that the application exists.
	if (getFlagAsInteger(settings, U"SkipIfBinaryExists") && file_getEntryType(fullProgramPath) == EntryType::File) {
		// SkipIfBinaryExists was active and the binary exists, so abort here to avoid redundant work.
		printText(U"Skipping build of ", projectFilePath, U" because the SkipIfBinaryExists flag was given and ", fullProgramPath, U" was found.\n");
		return;
	}
	// Once we are done finding all source files, we can resolve the dependencies to create a graph connected by indices.
	resolveDependencies(context);
	if (getFlagAsInteger(settings, U"ListDependencies")) {
		printDependencies(context);
	}
	generateCompilationScript(output, context, settings, fullProgramPath);
}

// Using a folder path and input arguments for all projects.
void buildProjects(ScriptTarget &output, const ReadableString &projectFolderPath, Machine &settings) {
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

void build(ScriptTarget &output, const ReadableString &projectPath, Machine &settings) {
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

void argumentsToSettings(Machine &settings, const List<String> &arguments, int64_t firstArgument) {
	for (int a = firstArgument; a < arguments.length(); a++) {
		String argument = arguments[a];
		int64_t assignmentIndex = string_findFirst(argument, U'=');
		if (assignmentIndex == -1) {
			assignValue(settings, argument, U"1", true);
			printText(U"Assigning ", argument, U" to 1 from input argument.\n");
		} else {
			String key = string_removeOuterWhiteSpace(string_before(argument, assignmentIndex));
			String value = string_removeOuterWhiteSpace(string_after(argument, assignmentIndex));
			assignValue(settings, key, value, true);
			printText(U"Assigning ", key, U" to ", value, U" from input argument.\n");
		}
	}
}
