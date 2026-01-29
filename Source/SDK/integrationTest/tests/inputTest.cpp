
// TODO: Include settings to adapt instructions for specific operating systems, such as control click and command click on MacOS and F keys for insert and pause.

#include "inputTest.h"

using namespace dsr;

#define TASK_IS(INDEX) context.taskIndex == INDEX
#define EVENT_TYPE_IS(TYPE) event.mouseEventType == MouseEventType::TYPE

void inputTests_populate(List<Test> &target, int32_t buttonCount, bool relative, bool verticalScroll) {
	if (buttonCount >= 3) {
		target.pushConstruct(
			U"Mouse button test"
		,
			[](AlignedImageRgbaU8 &canvas, TestContext &context) {
				image_fill(canvas, ColorRgbaI32(255, 255, 255, 255));
				context.drawAides(canvas);
				if (TASK_IS(0)) {
					font_printLine(canvas, font_getDefault(), U"Press down the left mouse button.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
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
				sendWarning(U"Detected a keyboard event with ", event.dsrKey, U" instead of a mouse button!\n");
			}
		,
			false
		);
	} else {
		sendWarning(U"Skipped the mouse button test due to settings.\n");
	}

	if (buttonCount >= 1) {
		target.pushConstruct(
			U"Mouse drag test"
		,
			[](AlignedImageRgbaU8 &canvas, TestContext &context) {
				image_fill(canvas, ColorRgbaI32(255, 255, 255, 255));
				context.drawAides(canvas);
				if (TASK_IS(0)) {
					font_printLine(canvas, font_getDefault(), U"Hover the cursor over the window.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(1)) {
					font_printLine(canvas, font_getDefault(), U"Press down the left mouse key.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(2)) {
					font_printLine(canvas, font_getDefault(), U"Drag the mouse over the window with the left key pressed down.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(3)) {
					font_printLine(canvas, font_getDefault(), U"Release the left key.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				}
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
		sendWarning(U"Skipped the mouse button drag test due to settings.\n");
	}
	if (buttonCount >= 1 && verticalScroll) {
		target.pushConstruct(
			U"Mouse scroll test"
		,
			[](AlignedImageRgbaU8 &canvas, TestContext &context) {
				image_fill(canvas, ColorRgbaI32(255, 255, 255, 255));
				context.drawAides(canvas);
				// TODO: Show something moving in the background while scrolling to show the direction. Only pass once the end has been reached.
				if (TASK_IS(0)) {
					font_printLine(canvas, font_getDefault(), U"Scroll in the direction used to reach the top of a document by moving content down.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(1)) {
					font_printLine(canvas, font_getDefault(), U"Click when you are done scrolling up.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(2)) {
					font_printLine(canvas, font_getDefault(), U"Scroll in the direction used to reach the bottom of a document by moving content up.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
				} else if (TASK_IS(3)) {
					font_printLine(canvas, font_getDefault(), U"Click when you are done scrolling down.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
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
	target.pushConstruct(
		U"Keyboard test"
	,
		[](AlignedImageRgbaU8 &canvas, TestContext &context) {
			image_fill(canvas, ColorRgbaI32(255, 255, 255, 255));
			// TODO: Draw a keyboard showing which key is expected and which key was detected.
			if (context.taskIndex < DsrKey::DsrKey_Unhandled) {
				font_printLine(canvas, font_getDefault(), string_combine(U"Press down ", (DsrKey)context.taskIndex, U" on your physical or virtual keyboard."), IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
			}
		}
	,
		[](const MouseEvent& event, TestContext &context) {
			sendWarning(U"Detected a mouse event with ", event.key, U" instead of a keyboard event!\n");
		}
	,
		[](const KeyboardEvent& event, TestContext &context) {
			if (event.keyboardEventType == KeyboardEventType::KeyDown && event.dsrKey == context.taskIndex) {
				// TODO: Require both down and up events.
				if (context.taskIndex >= DsrKey::DsrKey_Unhandled - 1) {
					context.finishTest(Grade::Passed);
				} else {
					context.passTask();
					// Skip testing the escape key, because it is currently reserved for skipping tests.
					if (context.taskIndex == DsrKey::DsrKey_Escape) {
						context.passTask();
					}
				}
			}
		}
	,
		false
	);
}
