#ifndef TEST_TOOLS
#define TEST_TOOLS

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
static thread_local bool failed = false;

static const int PASSED = 0;
static const int FAILED = 1;

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
			failed = true;
		} else {
			// Expected error.
			if (!beginsWith(message, ExpectedErrorPrefix)) {
				string_sendMessage_default(string_combine(U"Unexpected message in error!\n\nMessage:\n", message, U"\n\nExpected prefix:\n", ExpectedErrorPrefix, U"\n\n"), MessageType::Error);
				failed = true;
			} else {
				string_sendMessage_default(U"*", MessageType::StandardPrinting);
			}
		}
	} else {
		// Forward everything to the default message handler.
		string_sendMessage_default(message, type);
	}
}

#define START_TEST(NAME) \
	int main() { \
		std::signal(SIGSEGV, [](int signal) { throwError(U"Segmentation fault!"); }); \
		string_assignMessageHandler(&messageHandler); \
		heap_startingApplication(); \
		printText(U"Running test \"", #NAME, "\": ");

#define END_TEST \
		printText(U" (done)\n"); \
		heap_terminatingApplication(); \
		return PASSED; \
	}

#define OP_EQUALS(A, B) ((A) == (B))
#define OP_NOT_EQUALS(A, B) ((A) != (B))
#define OP_LESSER(A, B) ((A) < (B))
#define OP_LESSER_OR_EQUAL(A, B) ((A) <= (B))
#define OP_GREATER(A, B) ((A) > (B))
#define OP_GREATER_OR_EQUAL(A, B) ((A) >= (B))

// These can be used instead of ASSERT_CRASH to handle multiple template arguments that are not enclosed within ().
#define BEGIN_CRASH(PREFIX) \
	ExpectedErrorPrefix = PREFIX;
#define END_CRASH \
	ExpectedErrorPrefix = ""; \
	if (failed) return FAILED;

// Prefix is the expected start of the error message.
//   Just enough to know that we triggered the right error message.
#define ASSERT_CRASH(A, PREFIX) \
	BEGIN_CRASH(PREFIX); \
	(void)(A); \
	END_CRASH

#define ASSERT(CONDITION) \
	if (CONDITION) { \
		printText(U"*"); \
	} else { \
		printText(U"\n\n"); \
		printText(U"_______________________________ FAIL _______________________________\n"); \
		printText(U"\n"); \
		printText(U"Failed assertion!\nCondition: ", #CONDITION, U"\n"); \
		printText(U"____________________________________________________________________\n"); \
		return FAILED; \
	} \
	if (failed) return FAILED;
#define ASSERT_COMP(A, B, OP, OP_NAME) \
	if (OP(A, B)) { \
		printText(U"*"); \
	} else { \
		printText(U"\n\n"); \
		printText(U"_______________________________ FAIL _______________________________\n"); \
		printText(U"\n"); \
		printText(U"Condition: ", #A, " ", OP_NAME, U" ", #B, U"\n"); \
		printText((A), " ", OP_NAME, " ", (B), U" is false.\n"); \
		printText(U"____________________________________________________________________\n"); \
		printText(U"\n\n"); \
		return FAILED; \
	} \
	if (failed) return FAILED;
#define ASSERT_EQUAL(A, B) ASSERT_COMP(A, B, OP_EQUALS, "==")
#define ASSERT_NOT_EQUAL(A, B) ASSERT_COMP(A, B, OP_NOT_EQUALS, "!=")
#define ASSERT_LESSER(A, B) ASSERT_COMP(A, B, OP_LESSER, "<")
#define ASSERT_LESSER_OR_EQUAL(A, B) ASSERT_COMP(A, B, OP_LESSER_OR_EQUAL, "<=")
#define ASSERT_GREATER(A, B) ASSERT_COMP(A, B, OP_GREATER, ">")
#define ASSERT_GREATER_OR_EQUAL(A, B) ASSERT_COMP(A, B, OP_GREATER_OR_EQUAL, ">=")
#define ASSERT_NEAR(A, B) \
	if (nearValue(A, B)) { \
		printText(U"*"); \
	} else { \
		printText(U"\n\n"); \
		printText(U"_______________________________ FAIL _______________________________\n"); \
		printText(U"\n"); \
		printText(U"Condition: ", #A, U" = ", #B, U"\n"); \
		printText((A), " is not close enough to ", (B), U"\n"); \
		printText(U"____________________________________________________________________\n"); \
		printText(U"\n\n"); \
		return FAILED; \
	} \
	if (failed) return FAILED;

const dsr::String inputPath = dsr::string_combine(U"test", file_separator(), U"input", file_separator());
const dsr::String expectedPath = dsr::string_combine(U"test", file_separator(), U"expected", file_separator());

#endif

