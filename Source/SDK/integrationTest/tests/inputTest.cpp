
#include "inputTest.h"

using namespace dsr;

#define TASK_IS(INDEX) context.taskIndex == INDEX
#define EVENT_TYPE_IS(TYPE) event.mouseEventType == MouseEventType::TYPE

void inputTests_populate(List<Test> &target, int buttonCount, bool relative, bool verticalScroll) {
	if (buttonCount >= 1) {
		target.pushConstruct(
			U"Mouse drag test"
		,
			[](AlignedImageRgbaU8 &canvas, TestContext &context) {
				image_fill(canvas, ColorRgbaI32(255, 255, 255, 255));
				if (TASK_IS(0)) {
					font_printLine(canvas, font_getDefault(), U"Hover the cursor over the window.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(1)) {
					font_printLine(canvas, font_getDefault(), U"Press down the left mouse key.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(2)) {
					font_printLine(canvas, font_getDefault(), U"Drag the mouse over the window with the left key pressed down.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(3)) {
					font_printLine(canvas, font_getDefault(), U"Release the left key.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				}
				// TODO: Draw expanding circles from lowering and raising buttons together with names of the keys used.
				// TODO: Draw fading lines from move events.
				// TODO: Make a reusable drawing system to be enabled or disabled as a reusable mouse visualization feature.
			}
		,
			[](const MouseEvent& event, TestContext &context) {
				if (TASK_IS(0) && EVENT_TYPE_IS(MouseMove) && !context.leftMouseDown && !context.middleMouseDown && !context.rightMouseDown) {
					context.passTask();
				} else if (TASK_IS(1) && EVENT_TYPE_IS(MouseDown)) {
					if (event.key == MouseKeyEnum::Left) {
						context.passTask();
					} else {
						// TODO: Say which key was triggered instead and suggest skipping with escape if it can not be found.
					}
				} else if (TASK_IS(2) && EVENT_TYPE_IS(MouseMove) && context.leftMouseDown) {
					context.passTask();
				} else if (TASK_IS(3) && EVENT_TYPE_IS(MouseUp)) {
					if (event.key == MouseKeyEnum::Left) {
						context.finishTest(Grade::Passed);
					} else {
						// TODO: Say which key was triggered instead and suggest skipping with escape if it can not be found.
					}
				}
			}
		,
			[](const KeyboardEvent& event, TestContext &context) {}
		,
			false
		);
	} else {
		sendWarning(U"Skipped the left mouse button test, because no mouse buttons were told to be available in the call to inputTests_populate.");
	}
	// TODO: Create tests for other mouse buttons by reusing code in functions.
	// TODO: Create tests for keyboards, that display pressed physical keys on a QWERTY keyboard and unicode values of characters.
	//       The actual printed values depend on language settings, so this might have to be checked manually.
}
