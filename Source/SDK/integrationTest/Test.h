
#include "../../DFPSR/includeFramework.h"

struct TestContext;

using DrawContextCallback = std::function<void(dsr::AlignedImageRgbaU8 &canvas, TestContext &context)>;
using MouseContextCallback = std::function<void(const dsr::MouseEvent &event, TestContext &context)>;
using KeyboardContextCallback = std::function<void(const dsr::KeyboardEvent &event, TestContext &context)>;

enum class Grade {
	Waiting,
	Passed,
	Skipped,
	Failed
};

dsr::String& string_toStreamIndented(dsr::String& target, const Grade& grade, const dsr::ReadableString& indentation);

struct Test {
	dsr::String name;
	DrawContextCallback drawEvent;
	MouseContextCallback mouseCallback;
	KeyboardContextCallback keyboardCallback;
	bool activeDrawing;
	Grade result = Grade::Waiting;
	Test(const dsr::ReadableString& name, const DrawContextCallback &drawEvent, const MouseContextCallback &mouseCallback, const KeyboardContextCallback &keyboardCallback, bool activeDrawing)
	: name(name), drawEvent(drawEvent), mouseCallback(mouseCallback), keyboardCallback(keyboardCallback), activeDrawing(activeDrawing) {}
};

struct TestContext {
	dsr::List<Test> tests;

	// Each test consists of one or more tasks to pass.
	int32_t testIndex = 0; // Each test index refers to a Test in tests to be completed.
	int32_t taskIndex = 0; // To avoid cluttering the summary with lots of small tests, tests are divided into smaller tasks.

	// Call when completing a task but not a whole test.
	void passTask();

	// Call when completing a test.
	void finishTest(Grade result);

	void drawAides(dsr::AlignedImageRgbaU8 &canvas);

	bool leftMouseDown = false;
	bool middleMouseDown = false;
	bool rightMouseDown = false;

	dsr::IVector2D lastPosition;

	TestContext() {}
};
