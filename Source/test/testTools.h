#ifndef TEST_TOOLS
#define TEST_TOOLS

#include "../DFPSR/includeFramework.h"

using namespace dsr;

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

#define START_TEST(NAME) int main() { printText("Running test \"", #NAME, "\": ");
#define END_TEST printText(" (done)\n"); return PASSED; }

#define OP_EQUALS_STRING(A, B) (string_match(A, B))
#define OP_EQUALS(A, B) ((A) == (B))
#define OP_NOT_EQUALS(A, B) ((A) != (B))
#define OP_LESSER(A, B) ((A) < (B))
#define OP_LESSER_OR_EQUAL(A, B) ((A) <= (B))
#define OP_GREATER(A, B) ((A) > (B))
#define OP_GREATER_OR_EQUAL(A, B) ((A) >= (B))

#define ASSERT(CONDITION) \
	if (CONDITION) { \
		printText("*"); \
	} else { \
		printText("\n\n"); \
		printText("_______________________________ FAIL _______________________________\n"); \
		printText("\n"); \
		printText("Failed assertion!\nCondition: ", #CONDITION, "\n"); \
		printText("____________________________________________________________________\n"); \
		return FAILED; \
	}
#define ASSERT_COMP(A, B, OP, OP_NAME) \
	if (OP(A, B)) { \
		printText("*"); \
	} else { \
		printText("\n\n"); \
		printText("_______________________________ FAIL _______________________________\n"); \
		printText("\n"); \
		printText("Condition: ", #A, " ", OP_NAME, " ", #B, "\n"); \
		printText((A), " ", OP_NAME, " ", (B), " is false.\n"); \
		printText("____________________________________________________________________\n"); \
		printText("\n\n"); \
		return FAILED; \
	}
#define ASSERT_MATCH(A, B) ASSERT_COMP(A, B, OP_EQUALS_STRING, "matches")
#define ASSERT_EQUAL(A, B) ASSERT_COMP(A, B, OP_EQUALS, "==")
#define ASSERT_NOT_EQUAL(A, B) ASSERT_COMP(A, B, OP_NOT_EQUALS, "!=")
#define ASSERT_LESSER(A, B) ASSERT_COMP(A, B, OP_LESSER, "<")
#define ASSERT_LESSER_OR_EQUAL(A, B) ASSERT_COMP(A, B, OP_LESSER_OR_EQUAL, "<=")
#define ASSERT_GREATER(A, B) ASSERT_COMP(A, B, OP_GREATER, ">")
#define ASSERT_GREATER_OR_EQUAL(A, B) ASSERT_COMP(A, B, OP_GREATER_OR_EQUAL, ">=")
#define ASSERT_CRASH(A) \
try { \
	(void)(A); \
	return FAILED; \
} catch(...) {}
#define ASSERT_NEAR(A, B) \
	if (nearValue(A, B)) { \
		printText("*"); \
	} else { \
		printText("\n\n"); \
		printText("_______________________________ FAIL _______________________________\n"); \
		printText("\n"); \
		printText("Condition: ", #A, " ≈ ", #B, "\n"); \
		printText((A), " is not close enough to ", (B), "\n"); \
		printText("____________________________________________________________________\n"); \
		printText("\n\n"); \
		return FAILED; \
	}

const dsr::String inputPath = dsr::string_combine(U"test", file_separator(), U"input", file_separator());
const dsr::String expectedPath = dsr::string_combine(U"test", file_separator(), U"expected", file_separator());

#endif

