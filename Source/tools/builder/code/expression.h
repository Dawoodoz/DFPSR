
#ifndef DSR_BUILDER_EXPRESSION_MODULE
#define DSR_BUILDER_EXPRESSION_MODULE

#include "../../../DFPSR/api/stringAPI.h"

// The expression module is a slow but generic system for evaluating expressions where all data is stored as strings for simplicity.
//   No decimal numbers allowed, because it requires both human readable syntax and full determinism without precision loss.

// TODO: Move tokenization from Machine.cpp to expression.cpp

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

struct Symbol {
	dsr::String token;
	bool atomic; // Atomic symbols can affect tokenization, the other keywords have to be separated by whitespace or other symbols.
	POIndex operations[3]; // prefix, infix and postfix
	int32_t depthOffset;
	Symbol(const dsr::ReadableString &token, bool atomic, int32_t depthOffset = 0);
};

struct ExpressionSyntax {
	dsr::List<Symbol> symbols;
	dsr::List<Precedence> precedences;
	ExpressionSyntax();
};

dsr::String expression_unwrapIfNeeded(const dsr::ReadableString &text);

dsr::ReadableString expression_getToken(const dsr::List<dsr::String> &tokens, int64_t index);

int64_t expression_interpretAsInteger(const dsr::ReadableString &value);

dsr::String expression_evaluate(const dsr::List<dsr::String> &tokens, std::function<dsr::String(dsr::ReadableString)> identifierEvaluation);
dsr::String expression_evaluate(const dsr::List<dsr::String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex, std::function<dsr::String(dsr::ReadableString)> identifierEvaluation);
dsr::String expression_evaluate(const dsr::List<dsr::String> &tokens, int64_t startTokenIndex, int64_t endTokenIndex, const ExpressionSyntax &syntax, std::function<dsr::String(dsr::ReadableString)> identifierEvaluation);

void expression_runRegressionTests();

#endif
