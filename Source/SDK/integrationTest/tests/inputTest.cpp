
#include "inputTest.h"

using namespace dsr;

#define TASK_IS(INDEX) context.taskIndex == INDEX
#define EVENT_TYPE_IS(TYPE) event.mouseEventType == MouseEventType::TYPE

void inputTests_populate(List<Test> &target, int buttonCount, bool relative, bool verticalScroll) {
	if (buttonCount >= 3) {
		target.pushConstruct(
			U"Mouse button test"
		,
			[](AlignedImageRgbaU8 &canvas, TestContext &context) {
				image_fill(canvas, ColorRgbaI32(255, 255, 255, 255));
				if (TASK_IS(0)) {
					font_printLine(canvas, font_getDefault(), U"Press down the left mouse button, .", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(1)) {
					font_printLine(canvas, font_getDefault(), U"Release the left mouse button.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(2)) {
					font_printLine(canvas, font_getDefault(), U"Press down the right mouse button.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(3)) {
					font_printLine(canvas, font_getDefault(), U"Release the right mouse button.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(4)) {
					font_printLine(canvas, font_getDefault(), U"Press down the middle mouse button.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(5)) {
					font_printLine(canvas, font_getDefault(), U"Release the middle mouse button.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				}
				// TODO: Draw expanding circles from lowering and raising buttons together with names of the keys used.
				// TODO: Draw fading lines from move events.
				// TODO: Make a reusable drawing system to be enabled or disabled as a reusable mouse visualization feature.
			}
		,
			[](const MouseEvent& event, TestContext &context) {
				if (EVENT_TYPE_IS(MouseDown)) {
					if (TASK_IS(0) && event.key == MouseKeyEnum::Left) {
						context.passTask();
					} else if (TASK_IS(2) && event.key == MouseKeyEnum::Right) {
						context.passTask();
					} else if (TASK_IS(4) && event.key == MouseKeyEnum::Middle) {
						context.passTask();
					} else {
						sendWarning(U"Detected a different key!\n");
					}
				} else if (EVENT_TYPE_IS(MouseUp)) {
					if (TASK_IS(1) && event.key == MouseKeyEnum::Left) {
						context.passTask();
					} else if (TASK_IS(3) && event.key == MouseKeyEnum::Right) {
						context.passTask();
					} else if (TASK_IS(5) && event.key == MouseKeyEnum::Middle) {
						context.finishTest(Grade::Passed);
					} else {
						sendWarning(U"Detected a different key!\n");
					}
				}
			}
		,
			[](const KeyboardEvent& event, TestContext &context) {
				sendWarning(U"Detected a keyboard event with ", event.dsrKey, " instead of a mouse button!\n");
			}
		,
			false
		);
	} else {
		sendWarning(U"Skipped the left mouse button test due to settings.");
	}

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
		sendWarning(U"Skipped the left mouse button test due to settings.");
	}
	if (buttonCount >= 1 && verticalScroll) {
		target.pushConstruct(
			U"Mouse scroll test"
		,
			[](AlignedImageRgbaU8 &canvas, TestContext &context) {
				image_fill(canvas, ColorRgbaI32(255, 255, 255, 255));
				// TODO: Show something moving in the background while scrolling to show the direction. Only pass once the end has been reached.
				if (TASK_IS(0)) {
					font_printLine(canvas, font_getDefault(), U"Scroll in the direction used to reach the top of a document by moving content down.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(1)) {
					font_printLine(canvas, font_getDefault(), U"Click when you are down scrolling up.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(2)) {
					font_printLine(canvas, font_getDefault(), U"Scroll in the direction used to reach the bottom of a document by moving content up.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(3)) {
					font_printLine(canvas, font_getDefault(), U"Click when you are down scrolling down.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				}
			}
		,
			[](const MouseEvent& event, TestContext &context) {
				// Due to many laptops having the scroll direction inverted by default so that draging down scrolls up
				// Comparing how scrolling works in external text editors would be a useful addition to this test,
				// because many users probably don't know what is scrolling up and what is scrolling down when the direction is inverted on a track-pad.
				if (TASK_IS(0) && EVENT_TYPE_IS(Scroll)) {
					if (event.key == MouseKeyEnum::ScrollUp) {
						context.passTask();
					} else if (event.key == MouseKeyEnum::ScrollDown) {
						// Just give a warning, because this test can be very
						sendWarning(U"Scroll down was detected when attempting to scroll up. Compare the scrolling direction of a textbox with an external text editor to ensure consistent behavior.\n");
					}
				} else if (TASK_IS(1) && EVENT_TYPE_IS(MouseDown)) {
					context.passTask();
				} else if (TASK_IS(2) && EVENT_TYPE_IS(Scroll)) {
					if (event.key == MouseKeyEnum::ScrollDown) {
						context.passTask();
					} else if (event.key == MouseKeyEnum::ScrollUp) {
						sendWarning(U"Scroll up was detected when attempting to scroll down. Compare the scrolling direction of a textbox with an external text editor to ensure consistent behavior.\n");
					}
				} else if (TASK_IS(3) && EVENT_TYPE_IS(MouseDown)) {
					context.finishTest(Grade::Passed);
				}
			}
		,
			[](const KeyboardEvent& event, TestContext &context) {}
		,
			false
		);
	} else {
		sendWarning(U"Skipped the vertical scroll test due to settings.\n");
	}
	// TODO: Create tests for other mouse buttons by reusing code in functions.
	// TODO: Create tests for keyboards, that display pressed physical keys on a QWERTY keyboard and unicode values of characters.
	//       The actual printed values depend on language settings, so this might have to be checked manually.
}
