
#include "Machine.h"
#include "expression.h"
#include "../../../DFPSR/api/fileAPI.h"

using namespace dsr;

#define STRING_EXPR(FIRST_TOKEN, LAST_TOKEN) evaluateExpression(target, tokens, FIRST_TOKEN, LAST_TOKEN)
#define INTEGER_EXPR(FIRST_TOKEN, LAST_TOKEN) expression_interpretAsInteger(STRING_EXPR(FIRST_TOKEN, LAST_TOKEN))
#define PATH_EXPR(FIRST_TOKEN, LAST_TOKEN) file_getTheoreticalAbsolutePath(STRING_EXPR(FIRST_TOKEN, LAST_TOKEN), fromPath)

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

// Copy inherited variables from parent to child.
static void inheritMachine(Machine &child, const Machine &parent) {
	for (int64_t v = 0; v < parent.variables.length(); v++) {
		String key = string_upperCase(parent.variables[v].key);
		if (parent.variables[v].inherited) {
			child.variables.push(parent.variables[v]);
		}
	}
}

static void interpretLine(SessionContext &output, Machine &target, List<String> &tokens, const dsr::ReadableString &fromPath) {
	if (tokens.length() > 0) {
		bool activeLine = target.activeStackDepth >= target.currentStackDepth;
		/*
		printText(activeLine ? U"interpret:" : U"ignore:");
		for (int64_t t = 0; t < tokens.length(); t++) {
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
				evaluateScript(output, target, importPath);
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
				target.crawlOrigins.push(PATH_EXPR(1, tokens.length() - 1));
			} else if (string_caseInsensitiveMatch(first, U"build")) {
				// Build one or more other projects from a project file or folder path, as dependencies.
				//   Having the same external project built twice during the same session is not allowed.
				// Evaluate arguments recursively, but let the analyzer do the work.
				Machine childSettings;
				inheritMachine(childSettings, target);
				String projectPath = file_getTheoreticalAbsolutePath(expression_unwrapIfNeeded(second), fromPath); // Use the second token as the folder path.
				argumentsToSettings(childSettings, tokens, 2); // Send all tokens after the second token as input arguments to buildProjects.
				printText("Building ", second, " from ", fromPath, " which is ", projectPath, "\n");
				target.otherProjectPaths.push(projectPath);
				target.otherProjectSettings.push(childSettings);
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

void evaluateScript(SessionContext &output, Machine &target, const ReadableString &scriptPath) {
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
	for (int64_t i = 0; i <= string_length(projectContent); i++) {
		DsrChar c = projectContent[i];
		// Treat end of file as a linebreak to simplify tokenization rules.
		if (c == U'\0') c == U'\n';
		// The null terminator does not really exist in projectContent,
		//   but dsr::String returns a null character safely when requesting a character out of bound,
		//   which allow interpreting the last line without duplicating code.
		if (c == U'\n' || c == U'\0') {
			// Comment removing everything else.
			flushToken(currentLine, currentToken);
			interpretLine(output, target, currentLine, projectFolderPath);
			commented = false; // Automatically end comments at end of line.
			quoted = false; // Automatically end quotes at end of line.
		} else if (c == U'\"') {
			quoted = !quoted;
			string_appendChar(currentToken, c);
		} else if (c == U'#') {
			// Comment removing everything else until a new line comes.
			flushToken(currentLine, currentToken);
			interpretLine(output, target, currentLine, projectFolderPath);
			commented = true;
		} else if (!commented) {
			if (quoted) {
				// Insert character into quote.
				string_appendChar(currentToken, c);
			} else {
				// TODO: Do the tokenization in the expression module to get the correct symbols.
				if (c == U'(' || c == U')' || c == U'[' || c == U']' || c == U'{' || c == U'}' || c == U'=' || c == U'.' || c == U',' || c == U'|' || c == U'!' || c == U'&' || c == U'+' || c == U'-' || c == U'*' || c == U'/' || c == U'\\') {
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

void argumentsToSettings(Machine &settings, const List<String> &arguments, int64_t firstArgument) {
	for (int64_t a = firstArgument; a < arguments.length(); a++) {
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
