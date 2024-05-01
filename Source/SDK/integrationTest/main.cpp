
// An integration test application to quickly go through the most essential features to test in new implementations inheriting BackendWindow in the windowManagers folder.
//   Instead of reading documentation with risk of missunderstanding something, this integration test should guide the developer through the stages and give hints on what is wrong and how to fix it.
//   It should be somewhat difficult to pass the test by accident without having integrated the media layer correctly with the operating system.

// Planned tests:
// * Update the image one frame at a time while showing digits, then move on to the next digit after having pressed the correct key on the keyboard.
//   By randomizing the order, one can see if the keys are mapped wrong or if the canvas upload is delayed to show old images.
//   One can also show a frame counter for easy debugging.
//   Might need to use a boolean flag to manually say when it is okay to update the canvas.
//   Any repaint or resize events during the time may prove problematic, because the canvas should be possible to update once without getting duplicate requests.
// * Before the real tests begin, have a mode where one can freely move the mouse and get ripple animations from pressing, scrolling and hovering.
//   See the names of buttons being pressed, so that one can quickly try things out before starting the test.
// * Press and hold a keyboard button for ten seconds without getting any up or down events in between.
//   Features for repeated key presses have to be filtered out, so that up and down is only triggered by physical up and down events.
// * Press all the keys on the keyboard and see them light up on a picture of a keyboard as you type.
//   Create a reusable component for handling keyboard input, which can also be used to bind keys in a game.
// * Ask if relative input (mouse/ball/trackpad) is available and skip tests if not available.
//   Enter full screen and rotate a 3D camera in a cube map sky by moving the cursor to the center of the window.
//   Is there any clean way to allocate temporary resources for 3D graphics without creating an error-prone mess of pointers and inheritance?

#include "../../DFPSR/includeFramework.h"
#include "tests/inputTest.h"

using namespace dsr;

Window window;
bool running = true;
TestContext context;

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Create a window
	window = window_create(U"Integration test", 800, 600);

	// Create tests.
	// TODO: Allow selecting which tests to add using settings and white which categories were skipped in the final summary.
	//       Can have a screen where one gets to check types of tests and available device features.
	//       Some might want to skip tests that require certain types of input.
	//       One should have a mouse with vertical scroll wheel and three buttons to test it all. (The scroll wheel can often be clicked on as a middle mouse button)
	//       The media layer currently does not support horizontal scrolling, because otherwise one would need a laptop with each operating system just to test it.
	//       It is however cheap and easy to attach a three button mouse.
	//       A stylus pen with only two buttons, no scroll and no relative input will only be able to perform some of the tests.
	inputTests_populate(context.tests);

	// Create finishing screen showing results.
	context.tests.pushConstruct(
		U"Summary"
	,
		[](AlignedImageRgbaU8 &canvas, TestContext &context) {
			image_fill(canvas, ColorRgbaI32(255, 255, 255, 255));
			font_printLine(canvas, font_getDefault(), U"Completed integration test.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
			// TODO: Print a summary with named tests and their grades in a colored table.
		}
	,
		[](const MouseEvent& event, TestContext &context) {}
	,
		[](const KeyboardEvent& event, TestContext &context) {}
	,
		false
	);

	// TODO: Move to a method taking window as the argument in Test.cpp.
	window_setMouseEvent(window, [](const MouseEvent& event) {
		if (event.mouseEventType == MouseEventType::MouseDown) {
			if (event.key == MouseKeyEnum::Left) {
				context.leftMouseDown = true;
			} else if (event.key == MouseKeyEnum::Middle) {
				context.middleMouseDown = true;
			} else if (event.key == MouseKeyEnum::Right) {
				context.rightMouseDown = true;
			}
		} else if (event.mouseEventType == MouseEventType::MouseUp) {
			if (event.key == MouseKeyEnum::Left) {
				context.leftMouseDown = false;
			} else if (event.key == MouseKeyEnum::Middle) {
				context.middleMouseDown = false;
			} else if (event.key == MouseKeyEnum::Right) {
				context.rightMouseDown = false;
			}
		}
		if (context.testIndex >= 0 && context.testIndex < context.tests.length()) {
			context.tests[context.testIndex].mouseCallback(event, context);
		}
	});
	window_setKeyboardEvent(window, [](const KeyboardEvent& event) {
		if (context.testIndex >= 0 && context.testIndex < context.tests.length()) {
			if (event.keyboardEventType == KeyboardEventType::KeyDown && event.dsrKey == DsrKey::DsrKey_Escape) {
				if (context.testIndex >= context.tests.length() - 1) {
					running = false;
				} else {
					context.finishTest(Grade::Skipped);
				}
			} else {
				context.tests[context.testIndex].keyboardCallback(event, context);
			}
		}
	});

	window_setCloseEvent(window, []() {
		running = false;
	});

	// Execute
	while(running) {
		if (context.tests[context.testIndex].activeDrawing) {
			window_executeEvents(window);
		} else {
			// Wait for actions
			while (!window_executeEvents(window)) {
				time_sleepSeconds(0.01);
			}
		}
		// Get the current canvas from the swap chain.
		AlignedImageRgbaU8 canvas = window_getCanvas(window);
		// Draw things to the canvas.
		if (context.testIndex >= 0 && context.testIndex < context.tests.length()) {
			context.tests[context.testIndex].drawEvent(canvas, context);
		}
		// Show the canvas.
		window_showCanvas(window);
	}
}
