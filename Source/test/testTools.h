#ifndef TEST_TOOLS
#define TEST_TOOLS

// TODO: Make it faster to crawl source by only including what is needed by the test.
#include "../DFPSR/includeFramework.h"
#include <csignal>

using namespace dsr;

static bool beginsWith(const ReadableString &message, const ReadableString &prefix) {
	// Reading a character out of bound safely returns \0 by value, so we can rely
	// on the null character to exit early if message is storter than prefix.
	for (int c = 0; c < string_length(prefix); c++) {
		if (message[c] != prefix[c]) {
			return false;
		}
	}
	return true;
}

static thread_local String ExpectedErrorPrefix;

inline bool nearValue(float a, float b) {
	return fabs(a - b) < 0.0001f;
}
inline bool nearValue(const FVector2D& a, const FVector2D& b) {
	return nearValue(a.x, b.x) && nearValue(a.y, b.y);
}
inline bool nearValue(const FVector3D& a, const FVector3D& b) {
	return nearValue(a.x, b.x) && nearValue(a.y, b.y) && nearValue(a.z, b.z);
}
inline bool nearValue(const FVector4D& a, const FVector4D& b) {
	return nearValue(a.x, b.x) && nearValue(a.y, b.y) && nearValue(a.z, b.z) && nearValue(a.w, b.w);
}

static void messageHandler(const ReadableString &message, MessageType type) {
	if (type == MessageType::Error) {
		if (string_length(ExpectedErrorPrefix) == 0) {
			// Unexpected error!
			string_sendMessage_default(message, MessageType::Error);
		} else {
			// Expected error.
			if (!beginsWith(message, ExpectedErrorPrefix)) {
				string_sendMessage_default(string_combine(U"Unexpected message in error!\n\nMessage:\n", message, U"\n\nExpected prefix:\n", ExpectedErrorPrefix, U"\n\n"), MessageType::Error);
			} else {
				string_sendMessage_default(U"*", MessageType::StandardPrinting);
			}
		}
	} else {
		// Forward everything to the default message handler.
		string_sendMessage_default(message, type);
	}
}

static void handleArguments(const List<String> &args) {
	for (int i = 1; i < args.length(); i++) {
		String key = string_upperCase(args[i]);
		String value;
		if (i + 1 < args.length()) {
			value = args[i + 1];
		}
		if (string_match(key, U"-P") || string_match(key, U"--PATH")) {
			file_setCurrentPath(value);
		}
	}
}

static thread_local String testName = U"Uninitialized test\n";
static thread_local String stateName = U"New thread\n";

#define START_TEST(NAME) \
DSR_MAIN_CALLER(dsrMain) \
void dsrMain(List<String> args) { \
	testName = #NAME; \
	stateName = U"While Assigning message handler"; \
	std::signal(SIGSEGV, [](int signal) { throwError(U"Segmentation fault from ", testName, U"! ", stateName); }); \
	string_assignMessageHandler(&messageHandler); \
	stateName = U"While handling arguments\n"; \
	handleArguments(args); \
	stateName = U"Test start\n"; \
	printText(U"Running test \"", #NAME, "\": ");

#define END_TEST \
	printText(U" (done)\n"); \
	stateName = U"After test end\n"; \
}

#define OP_EQUALS(A, B) ((A) == (B))
#define OP_NOT_EQUALS(A, B) ((A) != (B))
#define OP_LESSER(A, B) ((A) < (B))
#define OP_LESSER_OR_EQUAL(A, B) ((A) <= (B))
#define OP_GREATER(A, B) ((A) > (B))
#define OP_GREATER_OR_EQUAL(A, B) ((A) >= (B))

// These can be used instead of ASSERT_CRASH to handle multiple template arguments that are not enclosed within ().
#define BEGIN_CRASH(PREFIX) \
	ExpectedErrorPrefix = PREFIX; \
	stateName = string_combine(U"During expected crash starting with ", PREFIX, U"\n");
#define END_CRASH \
	ExpectedErrorPrefix = "";

// Prefix is the expected start of the error message.
//   Just enough to know that we triggered the right error message.
#define ASSERT_CRASH(A, PREFIX) \
	BEGIN_CRASH(PREFIX); \
	(void)(A); \
	END_CRASH \
	stateName = string_combine(U"After expected crash starting with ", PREFIX, U"\n");

#define ASSERT(CONDITION) \
	stateName = string_combine(U"While evaluating condition ", #CONDITION, U"\n"); \
	if (CONDITION) { \
		printText(U"*"); \
	} else { \
		stateName = string_combine(U"While reporting failure for condition ", #CONDITION, U"\n"); \
		throwError( \
			U"\n\n", \
			U"_______________________________ FAIL _______________________________\n", \
			U"\n", \
			U"Failed assertion!\nCondition: ", #CONDITION, U"\n", \
			U"____________________________________________________________________\n" \
		); \
	} \
	stateName = string_combine(U"After evaluating condition ", #CONDITION, U"\n");

#define ASSERT_COMP(A, B, OP, OP_NAME) \
	stateName = string_combine(U"While evaluating comparison ", #A, " ", OP_NAME, U" ", #B, U"\n"); \
	if (OP(A, B)) { \
		printText(U"*"); \
	} else { \
	stateName = string_combine(U"While reporting failure for comparison ", #A, " ", OP_NAME, U" ", #B, U"\n"); \
		throwError( \
			U"\n\n", \
			U"_______________________________ FAIL _______________________________\n", \
			U"\n", \
			U"Condition: ", #A, " ", OP_NAME, U" ", #B, U"\n", \
			(A), " ", OP_NAME, " ", (B), U" is false.\n", \
			U"____________________________________________________________________\n" \
		); \
	} \
	stateName = string_combine(U"After evaluating comparison ", #A, " ", OP_NAME, U" ", #B, U"\n");
#define ASSERT_EQUAL(A, B) ASSERT_COMP(A, B, OP_EQUALS, "==")
#define ASSERT_NOT_EQUAL(A, B) ASSERT_COMP(A, B, OP_NOT_EQUALS, "!=")
#define ASSERT_LESSER(A, B) ASSERT_COMP(A, B, OP_LESSER, "<")
#define ASSERT_LESSER_OR_EQUAL(A, B) ASSERT_COMP(A, B, OP_LESSER_OR_EQUAL, "<=")
#define ASSERT_GREATER(A, B) ASSERT_COMP(A, B, OP_GREATER, ">")
#define ASSERT_GREATER_OR_EQUAL(A, B) ASSERT_COMP(A, B, OP_GREATER_OR_EQUAL, ">=")
#define ASSERT_NEAR(A, B) \
	stateName = string_combine(U"While evaluating approximate comparison between ", #A, " and ", #B, U"\n"); \
	if (nearValue(A, B)) { \
		printText(U"*"); \
	} else { \
		stateName = string_combine(U"While reporting failure for approximate comparison between ", #A, " and ", #B, U"\n"); \
		throwError( \
			U"\n\n", \
			U"_______________________________ FAIL _______________________________\n", \
			U"\n", \
			U"Condition: ", #A, U" = ", #B, U"\n", \
			(A), " is not close enough to ", (B), U"\n", \
			U"____________________________________________________________________\n" \
		); \
	} \
	stateName = string_combine(U"After evaluating approximate comparison between ", #A, " and ", #B, U"\n");

const dsr::String inputPath = dsr::string_combine(U"test", file_separator(), U"input", file_separator());
const dsr::String expectedPath = dsr::string_combine(U"test", file_separator(), U"expected", file_separator());

#endif
