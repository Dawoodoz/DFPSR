
#include "Test.h"

using namespace dsr;

String& string_toStreamIndented(String& target, const Grade& grade, const ReadableString& indentation) {
	if (grade == Grade::Waiting) {
		string_append(target, indentation, U"Waiting");
	} else if (grade == Grade::Passed) {
		string_append(target, indentation, U"Passed");
	} else if (grade == Grade::Skipped) {
		string_append(target, indentation, U"Skipped");
	} else if (grade == Grade::Failed) {
		string_append(target, indentation, U"Failed");
	} else {
		string_append(target, indentation, U"?");
	}
	return target;
}

// Call when completing a task but not a whole test.
void TestContext::passTask() {
	taskIndex++;
}

void TestContext::finishTest(Grade result) {
	if (result == Grade::Passed) {
		printText(U"Passed \"", tests[testIndex].name, U"\"\n.");
	} else if (result == Grade::Skipped) {
		sendWarning(U"Skipped \"", tests[testIndex].name, U"\"\n.");
	} else if (result == Grade::Failed) {
		sendWarning(U"Failed \"", tests[testIndex].name, U"\"\n.");
	}
	tests[testIndex].result = result;
	testIndex++;
	taskIndex = 0;
}
