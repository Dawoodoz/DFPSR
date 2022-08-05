
#include "Machine.h"
#include "generator.h"
#include "../../DFPSR/api/fileAPI.h"

using namespace dsr;

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

static String unwrapIfNeeded(const dsr::ReadableString &value) {
	if (value[0] == U'\"') {
		return string_unmangleQuote(value);
	} else {
		return value;
	}
}

void assignValue(Machine &target, const dsr::ReadableString &key, const dsr::ReadableString &value) {
	int64_t existingIndex = findFlag(target, key);
	if (existingIndex == -1) {
		target.variables.pushConstruct(string_upperCase(key), unwrapIfNeeded(value));
	} else {
		target.variables[existingIndex].value = unwrapIfNeeded(value);
	}
}

static void flushToken(List<String> &targetTokens, String &currentToken) {
	if (string_length(currentToken) > 0) {
		targetTokens.push(currentToken);
		currentToken = U"";
	}
}

// Safe access for easy pattern matching.
static ReadableString getToken(List<String> &tokens, int index) {
	if (0 <= index && index < tokens.length()) {
		return tokens[index];
	} else {
		return U"";
	}
}

static int64_t interpretAsInteger(const dsr::ReadableString &value) {
	if (string_length(value) == 0) {
		return 0;
	} else {
		return string_toInteger(value);
	}
}

#define STRING_EXPR(FIRST_TOKEN, LAST_TOKEN) evaluateExpression(target, tokens, FIRST_TOKEN, LAST_TOKEN)
#define STRING_LEFT STRING_EXPR(startTokenIndex, opIndex - 1)
#define STRING_RIGHT STRING_EXPR(opIndex + 1, endTokenIndex)

#define INTEGER_EXPR(FIRST_TOKEN, LAST_TOKEN) interpretAsInteger(STRING_EXPR(FIRST_TOKEN, LAST_TOKEN))
#define INTEGER_LEFT INTEGER_EXPR(startTokenIndex, opIndex - 1)
#define INTEGER_RIGHT INTEGER_EXPR(opIndex + 1, endTokenIndex)

#define PATH_EXPR(FIRST_TOKEN, LAST_TOKEN) file_getTheoreticalAbsolutePath(STRING_EXPR(FIRST_TOKEN, LAST_TOKEN), fromPath)

#define MATCH_CIS(TOKEN) string_caseInsensitiveMatch(currentToken, TOKEN)
#define MATCH_CS(TOKEN) string_match(currentToken, TOKEN)

static String evaluateExpression(Machine &target, List<String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex) {
	if (startTokenIndex == endTokenIndex) {
		ReadableString first = getToken(tokens, startTokenIndex);
		if (string_isInteger(first)) {
			return first;
		} else if (first[0] == U'\"') {
			return string_unmangleQuote(first);
		} else {
			// Identifier defaulting to empty.
			return getFlag(target, first, U"");
		}
	} else {
		int64_t depth = 0;
		for (int64_t opIndex = 0; opIndex < tokens.length(); opIndex++) {
			String currentToken = tokens[opIndex];
			if (MATCH_CS(U"(")) {
				depth++;
			} else if (MATCH_CS(U")")) {
				depth--;
				if (depth < 0) throwError(U"Negative expression depth!\n");
			} else if (MATCH_CIS(U"and")) {
				return string_combine(INTEGER_LEFT && INTEGER_RIGHT);
			} else if (MATCH_CIS(U"or")) {
				return string_combine(INTEGER_LEFT || INTEGER_RIGHT);
			} else if (MATCH_CIS(U"xor")) {
				return string_combine((!INTEGER_LEFT) != (!INTEGER_RIGHT));
			} else if (MATCH_CS(U"+")) {
				return string_combine(INTEGER_LEFT + INTEGER_RIGHT);
			} else if (MATCH_CS(U"-")) {
				return string_combine(INTEGER_LEFT - INTEGER_RIGHT);
			} else if (MATCH_CS(U"*")) {
				return string_combine(INTEGER_LEFT * INTEGER_RIGHT);
			} else if (MATCH_CS(U"/")) {
				return string_combine(INTEGER_LEFT / INTEGER_RIGHT);
			} else if (MATCH_CS(U"<")) {
				return string_combine(INTEGER_LEFT < INTEGER_RIGHT);
			} else if (MATCH_CS(U">")) {
				return string_combine(INTEGER_LEFT > INTEGER_RIGHT);
			} else if (MATCH_CS(U">=")) {
				return string_combine(INTEGER_LEFT >= INTEGER_RIGHT);
			} else if (MATCH_CS(U"<=")) {
				return string_combine(INTEGER_LEFT <= INTEGER_RIGHT);
			} else if (MATCH_CS(U"==")) {
				return string_combine(INTEGER_LEFT == INTEGER_RIGHT);
			} else if (MATCH_CS(U"!=")) {
				return string_combine(INTEGER_LEFT != INTEGER_RIGHT);
			} else if (MATCH_CS(U"&")) {
				return string_combine(STRING_LEFT, STRING_RIGHT);
			}
		}
		if (depth != 0) throwError(U"Unbalanced expression depth!\n");
		if (string_match(tokens[startTokenIndex], U"(") && string_match(tokens[endTokenIndex], U")")) {
			return evaluateExpression(target, tokens, startTokenIndex + 1, endTokenIndex - 1);
		}
	}
	throwError(U"Failed to evaluate expression!\n");
	return U"?";
}

static void analyzeSource(const dsr::ReadableString &absolutePath) {
	EntryType pathType = file_getEntryType(absolutePath);
	if (pathType == EntryType::File) {
		printText(U"  Using source from ", absolutePath, U".\n");
		analyzeFromFile(absolutePath);
	} else if (pathType == EntryType::Folder) {
		// TODO: Being analyzing from each source file in the folder recursively.
		//       Each file that is already included will quickly be ignored.
		//       The difficult part is that exploring a folder returns files in non-deterministic order and GNU's compiler is order dependent.
		printText(U"  Searching for source code from the folder ", absolutePath, U" is not yet supported due to order dependent linking!\n");
	} else if (pathType == EntryType::SymbolicLink) {
		// Symbolic links can point to both files and folder, so we need to follow it and find out what it really is.
		analyzeSource(file_followSymbolicLink(absolutePath));
	}
}

static void interpretLine(Machine &target, List<String> &tokens, const dsr::ReadableString &fromPath) {
	if (tokens.length() > 0) {
		bool activeLine = target.activeStackDepth >= target.currentStackDepth;
		/*
		printText(activeLine ? U"interpret:" : U"ignore:");
		for (int t = 0; t < tokens.length(); t++) {
			printText(U" [", tokens[t], U"]");
		}
		printText(U"\n");
		*/
		ReadableString first = getToken(tokens, 0);
		ReadableString second = getToken(tokens, 1);
		if (activeLine) {
			// TODO: Implement elseif and else cases using a list as a virtual stack,
			//       to remember at which layer the else cases have already been consumed by a true evaluation.
			// TODO: Remember at which depth the script entered, so that importing something can't leave the rest inside of a dangling if or else by accident.
			if (string_caseInsensitiveMatch(first, U"import")) {
				// Get path relative to importing script's path.
				String importPath = PATH_EXPR(1, tokens.length() - 1);
				evaluateScript(target, importPath);
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
				analyzeSource(PATH_EXPR(1, tokens.length() - 1));
			} else if (string_caseInsensitiveMatch(first, U"linkerflag")) {
				target.linkerFlags.push(STRING_EXPR(1, tokens.length() - 1));
			} else if (string_caseInsensitiveMatch(first, U"compilerflag")) {
				target.compilerFlags.push(STRING_EXPR(1, tokens.length() - 1));
			} else if (string_caseInsensitiveMatch(first, U"message")) {
				// Print a message while evaluating the build script.
				//   This is not done while actually compiling, so it will not know if compilation and linking worked or not.
				printText(STRING_EXPR(1, tokens.length() - 1));
			} else {
				if (tokens.length() == 1) {
					// Mentioning an identifier without assigning anything will assign it to one as a boolean flag.
					assignValue(target, first, U"1");
				} else if (string_match(second, U"=")) {
					// TODO: Create in-place math and string operations with different types of assignments.
					//       Maybe use a different syntax beginning with a keyword?
					// TODO: Look for the assignment operator dynamically if references to collection elements are allowed as l-value expressions.
					// Using an equality sign replaces any previous value of the variable.
					assignValue(target, first, STRING_EXPR(2, tokens.length() - 1));
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

void evaluateScript(Machine &target, const ReadableString &scriptPath) {
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
			interpretLine(target, currentLine, projectFolderPath);
			commented = false; // Automatically end comments at end of line.
			quoted = false; // Automatically end quotes at end of line.
		} else if (c == U'\"') {
			quoted = !quoted;
			string_appendChar(currentToken, c);
		} else if (c == U'#') {
			// Comment removing everything else until a new line comes.
			flushToken(currentLine, currentToken);
			interpretLine(target, currentLine, projectFolderPath);
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
