﻿
#include "expression.h"
#include "../../../DFPSR/api/stringAPI.h"

using namespace dsr;

POIndex::POIndex() {}
POIndex::POIndex(int16_t precedenceIndex, int16_t operationIndex) : precedenceIndex(precedenceIndex), operationIndex(operationIndex) {}

Operation::Operation(int16_t symbolIndex, std::function<String(ReadableString, ReadableString)> action)
: symbolIndex(symbolIndex), action(action) {
}

static int16_t addOperation(ExpressionSyntax &targetSyntax, int16_t symbolIndex, std::function<String(ReadableString, ReadableString)> action) {
	int16_t precedenceIndex = targetSyntax.precedences.length() - 1;
	int16_t operationIndex = targetSyntax.precedences.last().operations.length();
	// TODO: Only allow assigning a symbol once per prefix, infix and postfix.
	targetSyntax.symbols[symbolIndex].operations[targetSyntax.precedences.last().notation] = POIndex(precedenceIndex, operationIndex);
	targetSyntax.precedences.last().operations.pushConstruct(symbolIndex, action);
	return operationIndex;
}

Precedence::Precedence(Notation notation, Associativity associativity)
: notation(notation), associativity(associativity) {}

Symbol::Symbol(const ReadableString &token, SymbolType symbolType, int32_t depthOffset, DsrChar endsWith, DsrChar escapes)
: token(token), symbolType(symbolType), depthOffset(depthOffset), endsWith(endsWith), escapes(escapes) {}

ReadableString expression_getToken(const List<String> &tokens, int64_t index, const ReadableString &outside) {
	if (0 <= index && index < tokens.length()) {
		return tokens[index];
	} else {
		return outside;
	}
}

int64_t expression_interpretAsInteger(const ReadableString &value) {
	if (string_length(value) == 0) {
		return 0;
	} else {
		return string_toInteger(value);
	}
}

String expression_unwrapIfNeeded(const ReadableString &text) {
	if (text[0] == U'\"') {
		return string_unmangleQuote(text);
	} else {
		return text;
	}
}

static int16_t createSymbol(ExpressionSyntax &targetSyntax, const ReadableString &token, SymbolType symbolType, int32_t depthOffset, DsrChar endsWith, DsrChar escapes) {
	int64_t oldCount = targetSyntax.symbols.length();
	if (oldCount >= 32767) throwError(U"Can't declare more than 32767 symbols in a syntax, because they are referenced using 16-bit integers!\n");
	if (string_length(token) < 1) throwError(U"Can't declare a symbol without any characters, because the empty symbol exists between every character!\n");
	if (symbolType != SymbolType::Keyword) {
		if (targetSyntax.keywordCount > 0) throwError(U"Can't declare atomic symbols after the first keyword!\n");
		if (targetSyntax.atomicCount > 0 && string_length(targetSyntax.symbols[oldCount - 1].token) < string_length(token)) {
			throwError(U"Each following atomic token must be shorter or equal to the previous atomic token, so that longest match first can be applied!\n");
		}
		targetSyntax.atomicCount++;
	} else {
		targetSyntax.keywordCount++;
	}
	targetSyntax.symbols.pushConstruct(token, symbolType, depthOffset, endsWith, escapes);
	return (int16_t)oldCount;
}

#define CREATE_KEYWORD(TOKEN) createSymbol(*this, TOKEN, SymbolType::Keyword, 0, -1, -1);
#define CREATE_ATOMIC(TOKEN) createSymbol(*this, TOKEN, SymbolType::Atomic, 0, -1, -1);
#define CREATE_LEFT(TOKEN) createSymbol(*this, TOKEN, SymbolType::Atomic, 1, -1, -1);
#define CREATE_RIGHT(TOKEN) createSymbol(*this, TOKEN, SymbolType::Atomic, -1, -1, -1);
#define CREATE_LITERAL(START_TOKEN, END_CHAR, ESCAPE_CHAR) createSymbol(*this, START_TOKEN, SymbolType::Atomic, 0, END_CHAR, ESCAPE_CHAR);
#define CREATE_VOID(TOKEN) createSymbol(*this, TOKEN, SymbolType::Nothing, 0, -1, -1);
#define CREATE_COMMENT(TOKEN, END_CHAR, ESCAPE_CHAR) createSymbol(*this, TOKEN, SymbolType::Nothing, 0, END_CHAR, ESCAPE_CHAR);

// TODO: Create a way to enter symbols, keywords and operations from the outside to define custom syntax.
//       * Using a file or list of symbols is the easiest way to enter them by sorting automatically, but makes it hard to connect the indices with anything useful.
//       * Using multiple calls to an API makes it difficult to sort atomic symbols automatically based on length.
ExpressionSyntax::ExpressionSyntax() {
	// Symbols must be entered with longest match first, so that they can be used for tokenization.
	// Length 2 symbols
	int16_t token_lesserEqual = CREATE_ATOMIC(U"<="); // Allowed because both < and = are infix operations, which can not end up on the left or right sides.
	int16_t token_greaterEqual = CREATE_ATOMIC(U">="); // Allowed because both > and = are infix operations, which can not end up on the left or right sides.
	int16_t token_equal = CREATE_ATOMIC(U"=="); // Allowed because = is an infix operation, which can not end up on the left or right sides.
	int16_t token_notEqual = CREATE_ATOMIC(U"!="); // Allowed because ! is a prefix and would not end up on the left side of an assignment.
	// Length 1 symbols
	int16_t token_plus = CREATE_ATOMIC(U"+");
	int16_t token_minus = CREATE_ATOMIC(U"-");
	int16_t token_star = CREATE_ATOMIC(U"*");
	int16_t token_forwardSlash = CREATE_ATOMIC(U"/");
	int16_t token_backSlash = CREATE_ATOMIC(U"\\");
	int16_t token_exclamation = CREATE_ATOMIC(U"!");
	int16_t token_lesser = CREATE_ATOMIC(U"<");
	int16_t token_greater = CREATE_ATOMIC(U">");
	int16_t token_ampersand = CREATE_ATOMIC(U"&");
	// TODO: Connect scopes to each other for matching
	int16_t token_leftParen = CREATE_LEFT(U"(");
	int16_t token_leftBracket = CREATE_LEFT(U"[");
	int16_t token_leftCurl = CREATE_LEFT(U"{");
	int16_t token_rightParen = CREATE_RIGHT(U")");
	int16_t token_rightBracket = CREATE_RIGHT(U"]");
	int16_t token_rightCurl = CREATE_RIGHT(U"}");
	// Breaking
	int16_t token_lineBreak = CREATE_ATOMIC(U"\n");
	// Nothing
	CREATE_VOID(U" ");
	CREATE_VOID(U"\t");
	CREATE_VOID(U"\v");
	CREATE_VOID(U"\f");
	CREATE_VOID(U"\r"); // \r\n becomes \n, \n\r becomes \n and \n remains the same. String only using \r to break lines need to be converted into \n linebreaks before use.
	// Special tokens
	int16_t token_comment = CREATE_COMMENT(U"#", U'\n', -1); // # will begin a comment until the end of the line, without any escape character.
	int16_t token_doubleQuote = CREATE_LITERAL(U"\"", U'\"', U'\\'); // " will begin a literal until the next " not preceded by \.
	// Keywords that are used in expressions
	int16_t token_logical_and = CREATE_KEYWORD(U"and");
	int16_t token_logical_or = CREATE_KEYWORD(U"or");
	int16_t token_logical_xor = CREATE_KEYWORD(U"xor");
	int16_t token_string_match = CREATE_KEYWORD(U"matches");
	// Unidentified tokens are treated as identifiers or values with index -1.
	// Unlisted keywords can still be tokenized and used for statements, just not used to perform operations in expressions.

	// Each symbol can be tied once to prefix, once to infix and once to postfix.
	this->precedences.pushConstruct(Notation::Prefix, Associativity::RightToLeft);
		// Unary negation
		addOperation(*this, token_minus, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine(-expression_interpretAsInteger(rhs));
		});
		// Unary logical not
		addOperation(*this, token_exclamation, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine((!expression_interpretAsInteger(rhs)) ? 1 : 0);
		});
	this->precedences.pushConstruct(Notation::Infix, Associativity::LeftToRight);
		// Infix integer multiplication
		addOperation(*this, token_star, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine(expression_interpretAsInteger(lhs) * expression_interpretAsInteger(rhs));
		});
		// Infix integer division
		addOperation(*this, token_forwardSlash, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine(expression_interpretAsInteger(lhs) / expression_interpretAsInteger(rhs));
		});
	this->precedences.pushConstruct(Notation::Infix, Associativity::LeftToRight);
		// Infix integer addition
		addOperation(*this, token_plus, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine(expression_interpretAsInteger(lhs) + expression_interpretAsInteger(rhs));
		});
		// Infix integer subtraction
		addOperation(*this, token_minus, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine(expression_interpretAsInteger(lhs) - expression_interpretAsInteger(rhs));
		});
	this->precedences.pushConstruct(Notation::Infix, Associativity::LeftToRight);
		// Infix integer lesser than comparison
		addOperation(*this, token_lesser, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine((expression_interpretAsInteger(lhs) < expression_interpretAsInteger(rhs)) ? 1 : 0);
		});
		// Infix integer greater than comparison
		addOperation(*this, token_greater, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine((expression_interpretAsInteger(lhs) > expression_interpretAsInteger(rhs)) ? 1 : 0);
		});
		// Infix integer lesser than or equal to comparison
		addOperation(*this, token_lesserEqual, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine((expression_interpretAsInteger(lhs) <= expression_interpretAsInteger(rhs)) ? 1 : 0);
		});
		// Infix integer greater than or equal to comparison
		addOperation(*this, token_greaterEqual, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine((expression_interpretAsInteger(lhs) >= expression_interpretAsInteger(rhs)) ? 1 : 0);
		});
	this->precedences.pushConstruct(Notation::Infix, Associativity::LeftToRight);
		// Infix case sensitive string match
		addOperation(*this, token_string_match, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine(string_match(lhs, rhs) ? 1 : 0);
		});
		// Infix integer equal to comparison
		addOperation(*this, token_equal, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine((expression_interpretAsInteger(lhs) == expression_interpretAsInteger(rhs)) ? 1 : 0);
		});
		// Infix integer not equal to comparison
		addOperation(*this, token_notEqual, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine((expression_interpretAsInteger(lhs) != expression_interpretAsInteger(rhs)) ? 1 : 0);
		});
	this->precedences.pushConstruct(Notation::Infix, Associativity::LeftToRight);
		// Infix logical and
		addOperation(*this, token_logical_and, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine((expression_interpretAsInteger(lhs) && expression_interpretAsInteger(rhs)) ? 1 : 0);
		});
	this->precedences.pushConstruct(Notation::Infix, Associativity::LeftToRight);
		// Infix logical inclusive or
		addOperation(*this, token_logical_or, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine((expression_interpretAsInteger(lhs) || expression_interpretAsInteger(rhs)) ? 1 : 0);
		});
		// Infix logical exclusive or
		addOperation(*this, token_logical_xor, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine((!expression_interpretAsInteger(lhs) != !expression_interpretAsInteger(rhs)) ? 1 : 0);
		});
	this->precedences.pushConstruct(Notation::Infix, Associativity::LeftToRight);
		// Infix string concatenation
		addOperation(*this, token_ampersand, [](ReadableString lhs, ReadableString rhs) -> String {
			return string_combine(lhs, rhs);
		});
}

ExpressionSyntax defaultSyntax;

struct TokenInfo {
	int32_t depth = -1;
	int16_t symbolIndex = -1;
	TokenInfo() {}
	TokenInfo(int32_t depth, int16_t symbolIndex)
	: depth(depth), symbolIndex(symbolIndex) {}
};

static String debugTokens(const List<TokenInfo> &info, int64_t infoStart, const List<String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex) {
	String result;
	for (int64_t t = startTokenIndex; t <= endTokenIndex; t++) {
		int64_t infoIndex = t - infoStart;
		if (t > startTokenIndex) {
			string_appendChar(result, U' ');
		}
		string_append(result, tokens[t]);
	}
	string_append(result, U" : ");
	for (int64_t t = startTokenIndex; t <= endTokenIndex; t++) {
		int64_t infoIndex = t - infoStart;
		if (t > startTokenIndex) {
			string_appendChar(result, U' ');
		}
		string_append(result, "[", info[infoIndex].depth, ",", info[infoIndex].symbolIndex, ",", tokens[t], "]");
	}
	return result;
}

static String debugTokens(const List<String> &tokens) {
	String result;
	for (int64_t t = 0; t < tokens.length(); t++) {
		if (t > 0) {
			string_appendChar(result, U' ');
		}
		string_append(result, U"[", tokens[t], U"]");
	}
	return result;
}
static int16_t identifySymbol(const ReadableString &token, const ExpressionSyntax &syntax) {
	for (int64_t s = 0; s < syntax.symbols.length(); s++) {
		if (syntax.symbols[s].symbolType == SymbolType::Keyword) {
			// TODO: Make case insensitive optional for keywords.
			if (string_caseInsensitiveMatch(token, syntax.symbols[s].token)) {
				return s;
			}
		} else {
			if (string_match(token, syntax.symbols[s].token)) {
				return s;
			}
		}
	}
	return -1; // Pattern to resolve later.
}

// Returns true iff the symbol can be at the leftmost side of a sub-expression.
static bool validLeftmostSymbol(const Symbol &symbol) {
	if (symbol.depthOffset > 0) {
		return true; // ( [ { as the left side of a right hand side
	} else {
		return symbol.operations[Notation::Prefix].operationIndex != -1; // Accept prefix operations on the rightmost side
	}
}

// Returns true iff the symbol can be at the rightmost side of a sub-expression.
static bool validRightmostSymbol(const Symbol &symbol) {
	if (symbol.depthOffset < 0) {
		return true; // Accept ) ] } as the right side of a left hand side
	} else {
		return symbol.operations[Notation::Postfix].operationIndex != -1; // Accept postfix operations on the rightmost side
	}
}

// Returns true iff the symbol can be at the leftmost side of a sub-expression.
static bool validLeftmostToken(int16_t symbolIndex, const ExpressionSyntax &syntax) {
	return symbolIndex < 0 || validLeftmostSymbol(syntax.symbols[symbolIndex]);
}

// Returns true iff the symbol can be at the rightmost side of a sub-expression.
static bool validRightmostToken(int16_t symbolIndex, const ExpressionSyntax &syntax) {
	return symbolIndex < 0 || validRightmostSymbol(syntax.symbols[symbolIndex]);
}

// info is a list of additional information starting with info[0] at tokens[startTokenIndex]
// infoStart is the startTokenIndex of the root evaluation call
static String expression_evaluate_helper(const List<TokenInfo> &info, int64_t infoStart, int64_t currentDepth, const List<String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex, const ExpressionSyntax &syntax, std::function<String(ReadableString)> identifierEvaluation) {
	//printText(U"Evaluate: ", debugTokens(info, infoStart, tokens, startTokenIndex, endTokenIndex), U"\n");
	if (startTokenIndex == endTokenIndex) {
		ReadableString first = expression_getToken(tokens, startTokenIndex, U"");
		if (string_isInteger(first)) {
			return first;
		} else if (first[0] == U'\"') {
			// TODO: Let the caller unwrap strings.
			return string_unmangleQuote(first);
		} else {
			// Identifier defaulting to empty.
			return identifierEvaluation(first);
		}
	} else {
		// Find the outmost operation using recursive descent parsing, in which precedence and direction when going down is reversed relative to order of evaluation when going up.
		for (int64_t p = syntax.precedences.length() - 1; p >= 0; p--) {
			const Precedence *precedence = &(syntax.precedences[p]);
			int64_t leftScanBound = 0;
			int64_t rightScanBound = 0;
			if (precedence->notation == Notation::Prefix) {
				// A prefix can only be used at the start of the current sub-expression
				leftScanBound = startTokenIndex;
				rightScanBound = startTokenIndex;
				//printText("precendence = ", p, U" (prefix)\n");
			} else if (precedence->notation == Notation::Infix) {
				// Skip ends when looking for infix operations
				leftScanBound = startTokenIndex + 1;
				rightScanBound = endTokenIndex - 1;
				//printText("precendence = ", p, U" (infix)\n");
			} else if (precedence->notation == Notation::Postfix) {
				// A postfix can only be used at the end of the current sub-expression
				leftScanBound = endTokenIndex;
				rightScanBound = endTokenIndex;
				//printText("precendence = ", p, U" (postfix)\n");
			}
			int64_t opStep = (precedence->associativity == Associativity::LeftToRight) ? -1 : 1;
			int64_t opIndex = (precedence->associativity == Associativity::LeftToRight) ? rightScanBound : leftScanBound;
			int64_t stepCount = 1 + rightScanBound - leftScanBound;
			for (int64_t i = 0; i < stepCount; i++) {
				int64_t infoIndex = opIndex - infoStart;
				TokenInfo leftInfo = (opIndex <= startTokenIndex) ? TokenInfo() : info[infoIndex - 1];
				TokenInfo currentInfo = info[infoIndex];
				TokenInfo rightInfo = (opIndex >= endTokenIndex) ? TokenInfo() : info[infoIndex + 1];
				// Only match outmost at currentDepth.
				if (currentInfo.depth == currentDepth && currentInfo.symbolIndex > -1) {
					// If the current symbol is has an operation in the same notation and precedence, then grab that operation index.
					const Symbol *currentSymbol = &(syntax.symbols[currentInfo.symbolIndex]);
					if (currentSymbol->operations[precedence->notation].precedenceIndex == p) {
						// Resolve the common types of ambiguity that can quickly be resolved and let the other cases fail if the syntax is too ambiguous.
						bool validLeft = validRightmostToken(leftInfo.symbolIndex, syntax);
						bool validRight = validLeftmostToken(rightInfo.symbolIndex, syntax);
						bool valid = true;
						if (precedence->notation == Notation::Prefix) {
							if (!validRight) valid = false;
						} else if (precedence->notation == Notation::Infix) {
							if (!validLeft) valid = false;
							if (!validRight) valid = false;
						} else if (precedence->notation == Notation::Postfix) {
							if (!validLeft) valid = false;
						}
						if (valid) {
							const Operation *operation = &(precedence->operations[currentSymbol->operations[precedence->notation].operationIndex]);
							String lhs = (precedence->notation == Notation::Prefix) ? U"" : expression_evaluate_helper(info, infoStart, currentDepth, tokens, startTokenIndex, opIndex - 1, syntax, identifierEvaluation);
							String rhs = (precedence->notation == Notation::Postfix) ? U"" : expression_evaluate_helper(info, infoStart, currentDepth, tokens, opIndex + 1, endTokenIndex, syntax, identifierEvaluation);
							/*
							printText(U"Applied ", currentSymbol->token, "\n");
							printText(U"  currentDepth = ", currentDepth, U"\n");
							printText(U"  lhs = ", lhs, U"\n");
							printText(U"  rhs = ", rhs, U"\n");
							printText(U"  startTokenIndex = ", startTokenIndex, U"\n");
							printText(U"  leftScanBound = ", leftScanBound, U"\n");
							printText(U"  rightScanBound = ", rightScanBound, U"\n");
							printText(U"  endTokenIndex = ", endTokenIndex, U"\n");
							printText(U"  opStep = ", opStep, U"\n");
							printText(U"  opIndex = ", opIndex, U"\n");
							printText(U"  stepCount = ", stepCount, U"\n");
							printText(U"  notation = ", precedence->notation, U"\n");
							printText(U"  validLeft(", leftInfo.symbolIndex, U") = ", validLeft, U"\n");
							printText(U"  validRight(", rightInfo.symbolIndex, U") = ", validRight, U"\n");
							*/
							return operation->action(lhs, rhs);
						}
					}
				}
				opIndex += opStep;
			}
		}
		// TODO: Let the caller create a pattern matching operation for these combinations using longest match first.
		if (string_match(tokens[startTokenIndex], U"(") && string_match(tokens[endTokenIndex], U")")) {
			//printText(U"Unwrapping ()\n");
			return expression_evaluate_helper(info, infoStart, currentDepth + 1, tokens, startTokenIndex + 1, endTokenIndex - 1, syntax, identifierEvaluation);
		}
	}
	return U"<ERROR:Invalid expression>";
}

String expression_evaluate(const List<String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex, const ExpressionSyntax &syntax, std::function<String(ReadableString)> identifierEvaluation) {
	// Scan the whole expression once in the beginning and write useful information into a separate list.
	//   This allow handling tokens as plain lists of strings while still being able to number what they are.
	int32_t depth = 0;
	List<TokenInfo> info;
	for (int64_t opIndex = startTokenIndex; opIndex <= endTokenIndex; opIndex++) {
		String currentToken = tokens[opIndex];
		int16_t symbolIndex = identifySymbol(currentToken, syntax);
		int32_t depthOffet = (symbolIndex == -1) ? 0 : syntax.symbols[symbolIndex].depthOffset;
		if (depthOffet < 0) { // ) ] }
			depth += depthOffet;
			if (depth < 0) return U"<ERROR:Negative expression depth>";
		}
		info.pushConstruct(depth, symbolIndex);
		if (depthOffet > 0) { // ( [ {
			depth += depthOffet;
		}
	}
	if (depth != 0) return U"<ERROR:Unbalanced expression depth>";
	return expression_evaluate_helper(info, startTokenIndex, 0, tokens, startTokenIndex, endTokenIndex, syntax, identifierEvaluation);
}

String expression_evaluate(const List<String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex, std::function<String(ReadableString)> identifierEvaluation) {
	return expression_evaluate(tokens, startTokenIndex, endTokenIndex, defaultSyntax, identifierEvaluation);
}

String expression_evaluate(const List<String> &tokens, std::function<String(ReadableString)> identifierEvaluation) {
	return expression_evaluate(tokens, 0, tokens.length() - 1, defaultSyntax, identifierEvaluation);
}

// Atomic symbols are always case sensitive.
static bool matchAtomicFrom(const ReadableString &sourceText, int64_t location, const ReadableString &symbol) {
	for (int64_t l = 0; l < string_length(symbol); l++) {
		if (sourceText[location + l] != symbol[l]) {
			return false; // No match if a character deviated.
		}
	}
	return true; // Match if we found no contradicting characters.
}

void expression_tokenize(List<String> &targetTokens, const ReadableString &sourceText, const ExpressionSyntax &syntax) {
	//printText(U"expression_tokenize(", sourceText, U")\n");
	int64_t i = 0;
	int64_t keywordStart = 0;
	int64_t sourceLength = string_length(sourceText);
	while (i < sourceLength) {
		bool foundSymbol = false;
		for (int64_t s = 0; s < syntax.atomicCount; s++) {
			String startToken = syntax.symbols[s].token;
			if (matchAtomicFrom(sourceText, i, startToken)) {
				if (keywordStart < i) targetTokens.push(string_exclusiveRange(sourceText, keywordStart, i)); // Consume any previous keyword.
				int64_t startTokenLength = string_length(startToken);
				int64_t startIndex = i;
				int64_t exclusiveEndIndex = i + string_length(startToken);
				DsrChar endsWith = syntax.symbols[s].endsWith;
				DsrChar escapes = syntax.symbols[s].escapes;
				i += string_length(startToken);
				if (endsWith != -1) {
					// Find the end if the token is continuing.
					int64_t j;
					for (j = i; j < sourceLength; j++) {
						if (sourceText[j] == endsWith) {
							// Include the last character before ending
							j++;
							break;
						} else if (sourceText[j] == escapes) {
							// Jump past the next character when an escape character is met.
							j++;
						}
					}
					exclusiveEndIndex = j;
				}
				if (syntax.symbols[s].symbolType != SymbolType::Nothing) {
					// Include the token if it's not whitespace.
					targetTokens.push(string_exclusiveRange(sourceText, startIndex, exclusiveEndIndex));
				}
				i = exclusiveEndIndex;
				// Done identifying the symbol.
				foundSymbol = true;
				keywordStart = i;
				break;
			}
		}
		if (!foundSymbol) {
			i++;
		}
	}
	if (keywordStart < i) targetTokens.push(string_exclusiveRange(sourceText, keywordStart, i)); // Consume any last keyword.
	//printText(U"expression_tokenize finished with ", targetTokens.length(), " tokens\n");
}

void expression_tokenize(List<String> &targetTokens, const ReadableString &sourceText) {
	expression_tokenize(targetTokens, sourceText, defaultSyntax);
}

List<String> expression_tokenize(const ReadableString &sourceText, const ExpressionSyntax &syntax) {
	List<String> result;
	expression_tokenize(result, sourceText, syntax);
	return result;
}

List<String> expression_tokenize(const ReadableString &sourceText) {
	List<String> result;
	expression_tokenize(result, sourceText);
	return expression_tokenize(sourceText, defaultSyntax);
}

// -------- Regression tests --------

template<typename TYPE>
inline void appendToken(List<String>& target, const TYPE head) {
	target.push(head);
}
template<typename HEAD, typename... TAIL>
inline void appendToken(List<String>& target, const HEAD head, TAIL... tail) {
	appendToken(target, head);
	appendToken(target, tail...);
}
template<typename... ARGS>
inline List<String> combineTokens(ARGS... args) {
	List<String> result;
	appendToken(result, args...);
	return result;
}

static void expectResult(int64_t &errorCount, const ReadableString &result, const ReadableString &expected) {
	if (string_match(result, expected)) {
		printText(U"* Passed ", expected, U"\n");
	} else {
		printText(U"    - Failed ", expected, U" with unexpected ", result, U"\n");
		errorCount++;
	}
}

static void expectResult(int64_t &errorCount, const List<String> &result, const List<String> &expected) {
	if (result.length() != expected.length()) {
		printText(U"    - Failed\n    ", debugTokens(expected), U" with unexpected\n    ", debugTokens(result), U" of different token count\n");
		errorCount++;
		return;
	}
	for (int64_t t = 0; t < expected.length(); t++) {
		if (!string_match(expected[t], result[t])) {
			printText(U"    - Failed\n    ", debugTokens(expected), U" with unexpected\n    ", debugTokens(result), U"\n");
			errorCount++;
			return;
		}
	}
	printText(U"* Passed ", debugTokens(expected), U"\n");
}

void expression_runRegressionTests() {
	std::function<String(ReadableString)> context = [](ReadableString identifier) -> String {
		if (string_caseInsensitiveMatch(identifier, U"x")) {
			return U"5";
		} else if (string_caseInsensitiveMatch(identifier, U"doorCount")) {
			return U"48";
		} else if (string_caseInsensitiveMatch(identifier, U"temperature")) {
			return U"-18";
		} else {
			return U"<ERROR:Unresolved identifier>";
		}
	};
	/*for (int64_t s = 0; s < defaultSyntax.symbols.length(); s++) {
		printText(U"Symbol ", defaultSyntax.symbols[s].token, U"\n");
		if (validLeftmostToken(s, defaultSyntax)) printText(U"  Can be leftmost\n");
		if (validRightmostToken(s, defaultSyntax)) printText(U"  Can be rightmost\n");
	}*/
	int64_t ec = 0;
	// Tokenize
	printText(U"Tokenize test\n");
	expectResult(ec, expression_tokenize(U"0  "), combineTokens(U"0"));
	expectResult(ec, expression_tokenize(U"first line\nsecond line"), combineTokens(U"first", U"line", U"\n", U"second", U"line"));
	expectResult(ec, expression_tokenize(U"#A comment\nfirst line\nsecond line"), combineTokens(U"first", U"line", U"\n", U"second", U"line"));
	expectResult(ec, expression_tokenize(U"5+(7-8)"), combineTokens(U"5", U"+", U"(", U"7", U"-", U"8", U")"));
	expectResult(ec, expression_tokenize(U"identifier keyword"), combineTokens(U"identifier", U"keyword"));
	expectResult(ec, expression_tokenize(U"identifier+keyword"), combineTokens(U"identifier", U"+", U"keyword"));
	expectResult(ec, expression_tokenize(U"\t\tidentifier +  keyword "), combineTokens(U"identifier", U"+", U"keyword"));
	expectResult(ec, expression_tokenize(U"\" My string content \" \t+ \"My other string\""), combineTokens(U"\" My string content \"", U"+", U"\"My other string\""));
	expectResult(ec, expression_tokenize(U"\" My string content\n \" \t+ \"My other\n string\""), combineTokens(U"\" My string content\n \"", U"+", U"\"My other\n string\""));
	expectResult(ec, expression_tokenize(U"  \" My string content\n \"   # Comment \n + \"My other\n string\"  "), combineTokens(U"\" My string content\n \"", U"+", U"\"My other\n string\""));
	// Evaluate from tokens
	printText(U"Evaluate from tokens test\n");
	expectResult(ec, expression_evaluate(combineTokens(U""), context), U"<ERROR:Unresolved identifier>");
	expectResult(ec, expression_evaluate(combineTokens(U"0"), context), U"0");
	expectResult(ec, expression_evaluate(combineTokens(U"(", U"19", U")"), context), U"19");
	expectResult(ec, expression_evaluate(combineTokens(U"(", U"2", U"+", U"4", U")"), context), U"6");
	expectResult(ec, expression_evaluate(combineTokens(U"3"), context), U"3");
	expectResult(ec, expression_evaluate(combineTokens(U"-5"), context), U"-5");
	expectResult(ec, expression_evaluate(combineTokens(U"-", U"32"), context), U"-32");
	expectResult(ec, expression_evaluate(combineTokens(U"3", U"+", U"6"), context), U"9");
	expectResult(ec, expression_evaluate(combineTokens(U"x"), context), U"5");
	expectResult(ec, expression_evaluate(combineTokens(U"doorCount"), context), U"48");
	expectResult(ec, expression_evaluate(combineTokens(U"temperature"), context), U"-18");
	expectResult(ec, expression_evaluate(combineTokens(U"nonsense"), context), U"<ERROR:Unresolved identifier>");
	expectResult(ec, expression_evaluate(combineTokens(U"6", U"*", U"2", U"+", U"4"), context), U"16");
	expectResult(ec, expression_evaluate(combineTokens(U"4", U"+", U"6", U"*", U"2"), context), U"16");
	expectResult(ec, expression_evaluate(combineTokens(U"4", U"+", U"(", U"6", U"*", U"2", U")"), context), U"16");
	expectResult(ec, expression_evaluate(combineTokens(U"(", U"4", U"+", U"6", U")", U"*", U"2"), context), U"20");
	expectResult(ec, expression_evaluate(combineTokens(U"5", U"+", U"-", U"7"), context), U"-2");
	expectResult(ec, expression_evaluate(combineTokens(U"5", U"+", U"(", U"-", U"7", U")"), context), U"-2");
	expectResult(ec, expression_evaluate(combineTokens(U"5", U"+", U"(", U"-7", U")"), context), U"-2");
	expectResult(ec, expression_evaluate(combineTokens(U"5", U"+", U"-7"), context), U"-2");
	expectResult(ec, expression_evaluate(combineTokens(U"5", U"-", U"-", U"7"), context), U"12");
	expectResult(ec, expression_evaluate(combineTokens(U"5", U"&", U"-", U"7"), context), U"5-7");
	expectResult(ec, expression_evaluate(combineTokens(U"(", U"6", U"+", U"8", U")", U"/", U"(", U"9", U"-", U"2", U")"), context), U"2");
	expectResult(ec, expression_evaluate(combineTokens(U"(", U"6", U"+", U"8", U")", U"*", U"(", U"9", U"-", U"2", U")"), context), U"98");
	expectResult(ec, expression_evaluate(combineTokens(U"&", U"-", U"7"), context), U"<ERROR:Invalid expression>");
	expectResult(ec, expression_evaluate(combineTokens(U"(", U"-7"), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U")", U"3"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U"[", U"8"), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U"]", U"65"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U"{", U"12"), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U"}", U"0"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U"12", U"("), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U"2", U")"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U"-5", U"["), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U"6", U"]"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U"-47", U"{"), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U"645", U"}"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(combineTokens(U"5", U")", U"+", U"(", U"-7"), context), U"<ERROR:Negative expression depth>");
	// Tokenize and evaluate
	printText(U"Tokenize and evaluate test\n");
	expectResult(ec, expression_evaluate(expression_tokenize(U"0  "), context), U"0");
	expectResult(ec, expression_evaluate(expression_tokenize(U"(19)"), context), U"19");
	expectResult(ec, expression_evaluate(expression_tokenize(U"( 2+4)"), context), U"6");
	expectResult(ec, expression_evaluate(expression_tokenize(U"3"), context), U"3");
	expectResult(ec, expression_evaluate(expression_tokenize(U"- 5"), context), U"-5");
	expectResult(ec, expression_evaluate(expression_tokenize(U" -32"), context), U"-32");
	expectResult(ec, expression_evaluate(expression_tokenize(U"3+ 6"), context), U"9");
	expectResult(ec, expression_evaluate(expression_tokenize(U"x\t"), context), U"5");
	expectResult(ec, expression_evaluate(expression_tokenize(U"doorCount"), context), U"48");
	expectResult(ec, expression_evaluate(expression_tokenize(U"temperature"), context), U"-18");
	expectResult(ec, expression_evaluate(expression_tokenize(U"nonsense"), context), U"<ERROR:Unresolved identifier>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"6*2+4"), context), U"16");
	expectResult(ec, expression_evaluate(expression_tokenize(U"4+ 6*2"), context), U"16");
	expectResult(ec, expression_evaluate(expression_tokenize(U"4+(6* 2)"), context), U"16");
	expectResult(ec, expression_evaluate(expression_tokenize(U"(4+6)*2"), context), U"20");
	expectResult(ec, expression_evaluate(expression_tokenize(U"5+- 7"), context), U"-2");
	expectResult(ec, expression_evaluate(expression_tokenize(U"5+(-7)"), context), U"-2");
	expectResult(ec, expression_evaluate(expression_tokenize(U"5+(-7)"), context), U"-2");
	expectResult(ec, expression_evaluate(expression_tokenize(U"5+-7"), context), U"-2");
	expectResult(ec, expression_evaluate(expression_tokenize(U"5--7 "), context), U"12");
	expectResult(ec, expression_evaluate(expression_tokenize(U"5&-7"), context), U"5-7");
	expectResult(ec, expression_evaluate(expression_tokenize(U"(6+8)/(9-2)"), context), U"2");
	expectResult(ec, expression_evaluate(expression_tokenize(U"(6+8)*(9-2)"), context), U"98");
	expectResult(ec, expression_evaluate(expression_tokenize(U"	&-7"), context), U"<ERROR:Invalid expression>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"(-   7"), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U")3"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"[8"), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"]  65"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"{12"), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"}0"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"12("), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"2)"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"-5["), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"6]"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"-47 {"), context), U"<ERROR:Unbalanced expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"645}"), context), U"<ERROR:Negative expression depth>");
	expectResult(ec, expression_evaluate(expression_tokenize(U"5)+(-7"), context), U"<ERROR:Negative expression depth>");
	printText(U"Completed regression tests of expressions with ", ec, U" errors in total.\n");
}
