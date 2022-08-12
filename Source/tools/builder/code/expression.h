
#ifndef DSR_BUILDER_EXPRESSION_MODULE
#define DSR_BUILDER_EXPRESSION_MODULE

#include "../../../DFPSR/api/stringAPI.h"

// The expression module is a slow but generic system for evaluating expressions where all data is stored as strings for simplicity.
//   No decimal numbers allowed, because it requires both human readable syntax and full determinism without precision loss.

enum Notation {
	Prefix = 0,
	Infix = 1,
	Postfix = 2
};

enum Associativity {
	LeftToRight = 0,
	RightToLeft = 1
};

struct Operation {
	int16_t symbolIndex;
	std::function<dsr::String(dsr::ReadableString, dsr::ReadableString)> action;
	Operation(int16_t symbolIndex, std::function<dsr::String(dsr::ReadableString, dsr::ReadableString)> action);
};

struct Precedence {
	Notation notation;
	Associativity associativity;
	dsr::List<Operation> operations;
	Precedence(Notation notation, Associativity associativity);
};

struct POIndex {
	int16_t precedenceIndex = -1;
	int16_t operationIndex = -1;
	POIndex();
	POIndex(int16_t precedenceIndex, int16_t operationIndex);
};

enum class SymbolType {
	Nothing, // Whitespace does not produce any tokens, but counts as atomic.
	Atomic, // Will separate even directly connected to other tokens. These should not contain regular characters, to prevent cutting up identifiers.
	Keyword // The remains between atomic symbols and whilespace. Two keywords in a row needs to be separated by something else.
};

struct Symbol {
	dsr::String token;
	SymbolType symbolType;
	POIndex operations[3]; // prefix, infix and postfix
	int32_t depthOffset;
	dsr::DsrChar endsWith = -1; // If endsWith is not -1, the token will consume everything until the endsWith character not preceded by escapes is found.
	dsr::DsrChar escapes = -1;
	Symbol(const dsr::ReadableString &token, SymbolType symbolType, int32_t depthOffset, dsr::DsrChar endsWith, dsr::DsrChar escapes);
};

struct ExpressionSyntax {
	dsr::List<Symbol> symbols;
	dsr::List<Precedence> precedences;
	int16_t atomicCount = 0;
	int16_t keywordCount = 0;
	ExpressionSyntax();
};

dsr::String expression_unwrapIfNeeded(const dsr::ReadableString &text);

dsr::ReadableString expression_getToken(const dsr::List<dsr::String> &tokens, int64_t index, const dsr::ReadableString &outside);

int64_t expression_interpretAsInteger(const dsr::ReadableString &value);

dsr::String expression_evaluate(const dsr::List<dsr::String> &tokens, std::function<dsr::String(dsr::ReadableString)> identifierEvaluation);
dsr::String expression_evaluate(const dsr::List<dsr::String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex, std::function<dsr::String(dsr::ReadableString)> identifierEvaluation);
dsr::String expression_evaluate(const dsr::List<dsr::String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex, const ExpressionSyntax &syntax, std::function<dsr::String(dsr::ReadableString)> identifierEvaluation);

// Tokenizing into pure lists of strings is inefficient redundant work,
//   but a lot more reusable than a list of custom types hard-coded for a specific parser.
void expression_tokenize(dsr::List<dsr::String> &targetTokens, const dsr::ReadableString &sourceText, const ExpressionSyntax &syntax);
void expression_tokenize(dsr::List<dsr::String> &targetTokens, const dsr::ReadableString &sourceText);
dsr::List<dsr::String> expression_tokenize(const dsr::ReadableString &sourceText, const ExpressionSyntax &syntax);
dsr::List<dsr::String> expression_tokenize(const dsr::ReadableString &sourceText);

void expression_runRegressionTests();

#endif
