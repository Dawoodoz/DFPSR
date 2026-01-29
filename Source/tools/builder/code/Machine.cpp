
#include "Machine.h"
#include "expression.h"
#include "../../../DFPSR/api/fileAPI.h"

using namespace dsr;

#define STRING_EXPR(FIRST_TOKEN, LAST_TOKEN) evaluateExpression(target, tokens, FIRST_TOKEN, LAST_TOKEN)
#define INTEGER_EXPR(FIRST_TOKEN, LAST_TOKEN) expression_interpretAsInteger(STRING_EXPR(FIRST_TOKEN, LAST_TOKEN))
#define PATH_EXPR(FIRST_TOKEN, LAST_TOKEN) file_getTheoreticalAbsolutePath(STRING_EXPR(FIRST_TOKEN, LAST_TOKEN), fromPath)

static bool isUnique(const List<String> &list) {
	for (int i = 0; i < list.length() - 1; i++) {
		for (int j = i + 1; j < list.length(); j++) {
			if (string_match(list[i], list[j])) {
				return false;
			}
		}
	}
	return true;
}

static bool isUnique(const List<Flag> &list) {
	for (int i = 0; i < list.length() - 1; i++) {
		for (int j = i + 1; j < list.length(); j++) {
			if (string_match(list[i].key, list[j].key)) {
				return false;
			}
		}
	}
	return true;
}

void printSettings(const Machine &settings) {
	printText(U"    Project name: ", settings.projectName, U"\n");
	for (int64_t i = 0; i < settings.crawlOrigins.length(); i++) {
		printText(U"    Crawl origins ", settings.crawlOrigins[i], U"\n");
	}
	for (int64_t i = 0; i < settings.compilerFlags.length(); i++) {
		printText(U"    Compiler flag ", settings.compilerFlags[i], U"\n");
	}
	for (int64_t i = 0; i < settings.linkerFlags.length(); i++) {
		printText(U"    Linker flag ", settings.linkerFlags[i], U"\n");
	}
	for (int64_t i = 0; i < settings.frameworks.length(); i++) {
		printText(U"    Framework ", settings.frameworks[i], U"\n");
	}
	for (int64_t i = 0; i < settings.variables.length(); i++) {
		printText(U"    Variable ", settings.variables[i].key, U" = ", settings.variables[i].value, U"\n");
	}
}

void validateSettings(const Machine &settings, const dsr::ReadableString &eventDescription) {
	if (!isUnique(settings.compilerFlags)) {
		printText(U"Duplicate compiler flags:\n");
		printSettings(settings);
		throwError(U"Found duplicate compiler flags ", eventDescription, U"!\n");
	};
	if (!isUnique(settings.linkerFlags)) {
		printText(U"Duplicate linker flags:\n");
		printSettings(settings);
		throwError(U"Found duplicate linker flags ", eventDescription, U"!\n");
	};
	if (!isUnique(settings.frameworks)) {
		printText(U"Duplicate frameworks:\n");
		printSettings(settings);
		throwError(U"Found duplicate frameworks ", eventDescription, U"!\n");
	};
	if (!isUnique(settings.variables)) {
		printText(U"Duplicate variables:\n");
		printSettings(settings);
		throwError(U"Found duplicate variables ", eventDescription, U"!\n");
	};
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

static String evaluateExpression(Machine &target, const List<String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex) {
	for (int64_t t = startTokenIndex; t <= endTokenIndex; t++) {
		if (string_match(tokens[t], U"\n")) {
			throwError(U"Found a linebreak inside of an expression!");
		}
	}
	return expression_evaluate(tokens, startTokenIndex, endTokenIndex, [&target](ReadableString identifier) -> String {
		return getFlag(target, identifier, U"");
	});
}

// Copy inherited variables from parent to child.
void inheritMachine(Machine &child, const Machine &parent) {
	// Only take selected variables, such as the target platform's name.
	for (int64_t v = 0; v < parent.variables.length(); v++) {
		if (parent.variables[v].inherited) {
			child.variables.push(parent.variables[v]);
		}
	}
}

void cloneMachine(Machine &child, const Machine &parent) {
	// Inherit everything.
	for (int64_t v = 0; v < parent.variables.length(); v++) {
		child.variables.push(parent.variables[v]);
	}
	for (int64_t c = 0; c < parent.compilerFlags.length(); c++) {
		child.compilerFlags.push(parent.compilerFlags[c]);
	}
	for (int64_t l = 0; l < parent.linkerFlags.length(); l++) {
		child.linkerFlags.push(parent.linkerFlags[l]);
	}
	for (int64_t f = 0; f < parent.frameworks.length(); f++) {
		child.linkerFlags.push(parent.frameworks[f]);
	}
	for (int64_t o = 0; o < parent.crawlOrigins.length(); o++) {
		child.crawlOrigins.push(parent.crawlOrigins[o]);
	}
}

static bool validIdentifier(const dsr::ReadableString &identifier) {
	DsrChar first = identifier[0];
	if (!((U'a' <= first && first <= U'z') || (U'A' <= first && first <= U'Z'))) {
		return false;
	}
	for (int i = 1; i < string_length(identifier); i++) {
		DsrChar current = identifier[i];
		if (!((U'a' <= current && current <= U'z') || (U'A' <= current && current <= U'Z') || (U'0' <= current && current <= U'9'))) {
			return false;
		}
	}
	return true;
}

using NameFilter = std::function<bool(const ReadableString &filename)>;
static NameFilter generateFilterFromPattern(const dsr::ReadableString &pattern) {
	int64_t firstStar = string_findFirst(pattern, U'*');
	int64_t lastStar = string_findLast(pattern, U'*');
	if (firstStar == -1) {
		return [pattern](const ReadableString &filename) -> bool {
			return string_caseInsensitiveMatch(filename, pattern);
		};
	} else if (firstStar == lastStar) {
		String prefix = string_before(pattern, firstStar);
		String postfix = string_after(pattern, lastStar);
		int64_t preLength = string_length(prefix);
		int64_t postLength = string_length(postfix);
		int64_t minimumLength = preLength + postLength;
		return [prefix, postfix, preLength, postLength, minimumLength](const ReadableString &filename) -> bool {
			int64_t nameLength = string_length(filename);
			if (nameLength < minimumLength) {
				return false;
			} else {
				ReadableString foundPrefix = string_before(filename, preLength);
				ReadableString foundPostfix = string_from(filename, nameLength - postLength);
				return string_caseInsensitiveMatch(foundPrefix, prefix) && string_caseInsensitiveMatch(foundPostfix, postfix);
			}
		};
	} else {
		throwError(U"Can not use '", pattern, U"' as a name pattern, because the matching expression may not use more than one '*' character!\n");
		return [](const ReadableString &filename) -> bool {
			return false;
		};
	}
}

static void findFiles(const dsr::ReadableString &inPath, NameFilter filter, std::function<void(const ReadableString &path)> action) {
	if (!file_getFolderContent(inPath, [&filter, &action](const ReadableString& entryPath, const ReadableString& entryName, EntryType entryType) {
		if (entryType == EntryType::File) {
			if (filter(entryName)) {
				action(entryPath);
			}
		} else if (entryType == EntryType::Folder) {
			findFiles(entryPath, filter, action);
		}
	})) {
		printText(U"Failed to look for files in '", inPath, U"'\n");
	}
}

static void findFilesAsProjects(Machine &target, const dsr::ReadableString &inPath, const dsr::ReadableString &fromPattern) {
	printText(U"findFilesAsProjects: Looking for ", fromPattern, U" in ", inPath, U".\n");
	validateSettings(target, U"in the parent about to create projects from files");
	findFiles(inPath, generateFilterFromPattern(fromPattern), [&target](const ReadableString &path) {
		printText(U"Creating a temporary project for ", path, U"\n");		
		// List the file as a project.
		target.projectFromSourceFilenames.push(path);
		Machine allInputFlags(file_getPathlessName(path));
		cloneMachine(allInputFlags, target);
		target.projectFromSourceSettings.push(allInputFlags);
	});
}

// TODO: Improve error messages with line numbers and quoted content instead of just throwing errors.

static void interpretLine(Machine &target, const List<String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex, const dsr::ReadableString &fromPath) {
	// Automatically clamp to safe bounds.
	if (startTokenIndex < 0) startTokenIndex = 0;
	if (endTokenIndex >= tokens.length()) endTokenIndex = tokens.length() - 1;
	int64_t tokenCount = endTokenIndex - startTokenIndex + 1;
	if (tokenCount > 0) {
		bool activeLine = target.activeStackDepth >= target.currentStackDepth;
		/*
		printText(activeLine ? U"interpret:" : U"ignore:");
		for (int64_t t = startTokenIndex; t <= endTokenIndex; t++) {
			printText(U" [", tokens[t], U"]");
		}
		printText(U"\n");
		*/
		ReadableString first = expression_getToken(tokens, startTokenIndex, U"");
		ReadableString second = expression_getToken(tokens, startTokenIndex + 1, U"");
		if (activeLine) {
			// TODO: Implement elseif and else cases using a list as a virtual stack,
			//       to remember at which layer the else cases have already been consumed by a true evaluation.
			// TODO: Remember at which depth the script entered, so that importing something can't leave the rest inside of a dangling if or else by accident.
			if (string_caseInsensitiveMatch(first, U"import")) {
				// Get path relative to importing script's path.
				String importPath = PATH_EXPR(startTokenIndex + 1, endTokenIndex);
				evaluateScript(target, importPath);
				validateSettings(target, U"in target after importing a project head\n");
			} else if (string_caseInsensitiveMatch(first, U"if")) {
				// Being if statement
				bool active = INTEGER_EXPR(startTokenIndex + 1, endTokenIndex);
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
				target.crawlOrigins.push(PATH_EXPR(startTokenIndex + 1, endTokenIndex));
				validateSettings(target, U"in target after listing a crawl origin\n");
			} else if (string_caseInsensitiveMatch(first, U"projects")) {
				// TODO: Should it be possible to give the string content of variables as patterns and paths?
				//Projects from "*Test.cpp" in "tests"
				int currentTokenIndex = startTokenIndex + 1;
				String arg_from;
				String arg_in;
				while (currentTokenIndex < endTokenIndex) {
					ReadableString key = expression_getToken(tokens, currentTokenIndex, U"");
					ReadableString value = expression_getToken(tokens, currentTokenIndex + 1, U"");
					if (string_caseInsensitiveMatch(key, U"from")) {
						if (string_length(value) == 0) {
							throwError(U"Missing folder path after 'from' keyword in 'projects' command!\n");
						} else {
							printText(U"Using ", value, U" as the 'from' argument.\n");
							arg_from = string_unmangleQuote(value);
							// Consume both key and value.
							currentTokenIndex += 2;
						}
					} else if (string_caseInsensitiveMatch(key, U"in")) {
						if (string_length(value) == 0) {
							throwError(U"Missing file name pattern after 'in' keyword in 'projects' command!\n");
						} else {
							printText(U"Using ", value, U" as the 'in' argument.\n");
							arg_in = string_unmangleQuote(value);
							// Consume both key and value.
							currentTokenIndex += 2;
						}
					} else {
						throwError(U"Unexpected key '", key, U"' in 'projects' command!\n");
					}
				}
				if (string_length(arg_from) == 0 && string_length(arg_in) == 0) {
					throwError(U"Need 'from' and 'in' keywords in 'projects' command!\n");
				} else if (string_length(arg_from) == 0) {
					throwError(U"Missing 'from' keyword in 'projects' command!\n");
				} else if (string_length(arg_in) == 0) {
					throwError(U"Missing 'in' keywords in 'projects' command!\n");
				} else {
					findFilesAsProjects(target, file_combinePaths(fromPath, arg_in), arg_from);
				}
			} else if (string_caseInsensitiveMatch(first, U"build")) {
				// Build one or more other projects from a project file or folder path, as dependencies.
				//   Having the same external project built twice during the same session is not allowed.
				// Evaluate arguments recursively, but let the analyzer do the work.
				String projectPath = file_getTheoreticalAbsolutePath(expression_unwrapIfNeeded(second), fromPath); // Use the second token as the folder path.
				// The arguments may be for a whole folder of projects, so each project still need to clone its own settings.
				Machine sharedInputFlags(file_getPathlessName(projectPath));
				validateSettings(target, U"in the parent about to build a child project (build in interpretLine)");
				inheritMachine(sharedInputFlags, target);
				validateSettings(sharedInputFlags, U"in the parent after inheriting settings for a build child (build in interpretLine)");
				validateSettings(sharedInputFlags, U"in the child after inheriting settings as a build child (build in interpretLine)");
				argumentsToSettings(sharedInputFlags, tokens, startTokenIndex + 2, endTokenIndex); // Send all tokens after the second token as input arguments to buildProjects.
				validateSettings(sharedInputFlags, U"in the child after parsing arguments (build in interpretLine)");
				printText(U"Building ", second, U" from ", fromPath, U" which is ", projectPath, U"\n");
				target.otherProjectPaths.push(projectPath);
				target.otherProjectSettings.push(sharedInputFlags);
				validateSettings(target, U"in target after listing a child project\n");
			} else if (string_caseInsensitiveMatch(first, U"link")) {
				// Only the library name itself is needed, because the -l prefix can be added automatically.
				String libraryName = STRING_EXPR(startTokenIndex + 1, endTokenIndex);
				if (libraryName[0] == U'-' && (libraryName[1] == U'l' || libraryName[1] == U'L')) {
					// Avoid duplicating -l when it has already been included by accident.
					target.linkerFlags.push(libraryName);
				} else {
					// Insert the library name after -l when used correctly.
					target.linkerFlags.push(string_combine(U"-l", libraryName));
				}
				validateSettings(target, U"in target after adding a library\n");
			} else if (string_caseInsensitiveMatch(first, U"linkerflag")) {
				// For linker flags that are not used to link with a library.
				target.linkerFlags.push(STRING_EXPR(startTokenIndex + 1, endTokenIndex));
				validateSettings(target, U"in target after adding a linker flag\n");
			} else if (string_caseInsensitiveMatch(first, U"framework")) {
				// For linking with a framework. (MacOS feature in Clang where the name follows a separate -framework argument)
				target.frameworks.push(STRING_EXPR(startTokenIndex + 1, endTokenIndex));
				validateSettings(target, U"in target after adding a framework\n");
			} else if (string_caseInsensitiveMatch(first, U"compilerflag")) {
				target.compilerFlags.push(STRING_EXPR(startTokenIndex + 1, endTokenIndex));
				validateSettings(target, U"in target after adding a compiler flag\n");
			} else if (string_caseInsensitiveMatch(first, U"message")) {
				// Print a message while evaluating the build script.
				//   This is not done while actually compiling, so it will not know if compilation and linking worked or not.
				printText(STRING_EXPR(startTokenIndex + 1, endTokenIndex));
			} else {
				if (tokenCount == 1) {
					// Mentioning an identifier without assigning anything will assign it to one as a boolean flag.
					if (validIdentifier(first)) {
						assignValue(target, first, U"1", false);
					} else {
						throwError(U"The token ", first, U" is not a valid identifier for implicit assignment to one.\n");
					}
					validateSettings(target, U"in target after implicitly assigning a value to a variable\n");
				} else if (string_match(second, U"=")) {
					// TODO: Create in-place math and string operations with different types of assignments.
					//       Maybe use a different syntax beginning with a keyword?
					// TODO: Look for the assignment operator dynamically if references to collection elements are allowed as l-value expressions.
					// Using an equality sign replaces any previous value of the variable.
					if (validIdentifier(first)) {
						assignValue(target, first, STRING_EXPR(startTokenIndex + 2, endTokenIndex), false);
					} else {
						throwError(U"The token ", first, U" is not a valid identifier for assignments.\n");
					}
					validateSettings(target, U"in target after explicitly assigning a value to a variable\n");
				} else {
					String errorMessage = U"Failed to parse statement: ";
					for (int64_t t = startTokenIndex; t <= endTokenIndex; t++) {
						string_append(errorMessage, U" ", string_mangleQuote(tokens[t]));
					}
					string_append(errorMessage, U"\n");
					throwError(errorMessage);
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
}

void evaluateScript(Machine &target, const ReadableString &scriptPath) {
	//printText(U"Evaluating script at ", scriptPath, U"\n");
	//printSettings(target);
	if (file_getEntryType(scriptPath) != EntryType::File) {
		printText(U"The script path ", scriptPath, U" does not exist!\n");
	}
	// Each new script being imported will have its own simulated current path for accessing files and such.
	String projectFolderPath = file_getAbsoluteParentFolder(scriptPath);
	// Tokenize the document to handle string literals.
	String projectContent = string_load(scriptPath);
	List<String> tokens;
	expression_tokenize(tokens, projectContent);
	// Insert an extra linebreak at the end to avoid special cases for the last line.
	tokens.push(U"\n");
	// Segment tokens into logical lines and interpret one at a time.
	int64_t startTokenIndex = 0;
	for (int64_t t = 0; t < tokens.length(); t++) {
		if (string_match(tokens[t], U"\n")) {
			interpretLine(target, tokens, startTokenIndex, t - 1, projectFolderPath);
			startTokenIndex = t + 1;
		}
	}
	//printText(U"Evaluated script at ", scriptPath, U"\n");
	//printSettings(target);
}

void argumentsToSettings(Machine &settings, const List<String> &arguments, int64_t firstArgument, int64_t lastArgument) {
	//printText(U"argumentsToSettings:");
	//for (int64_t a = firstArgument; a <= lastArgument; a++) {
	//	printText(U" ", arguments[a]);
	//}
	//printText(U"\n");
	for (int64_t a = firstArgument; a <= lastArgument; a++) {
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
