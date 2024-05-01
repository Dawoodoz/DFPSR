
#include "inputTest.h"

using namespace dsr;

void inputTests_populate(List<Test> &target) {
	target.pushConstruct(
		U"Mouse drag test"
	,
		[](AlignedImageRgbaU8 &canvas, TestContext &context) {
			image_fill(canvas, ColorRgbaI32(255, 255, 255, 255));
			if (context.taskIndex == 0) {
				font_printLine(canvas, font_getDefault(), U"Hover the cursor over the window.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
			} else if (context.taskIndex == 1) {
				font_printLine(canvas, font_getDefault(), U"Press down the left mouse key.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
			} else if (context.taskIndex == 2) {
				font_printLine(canvas, font_getDefault(), U"Drag the mouse over the window with the left key pressed down.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
			} else if (context.taskIndex == 3) {
				font_printLine(canvas, font_getDefault(), U"Release the left key.", IVector2D(40, 40), ColorRgbaI32(0, 0, 0, 255));
			}
		}
	,
		[](const MouseEvent& event, TestContext &context) {
			if (context.taskIndex == 0 && event.mouseEventType == MouseEventType::MouseMove && !context.leftMouseDown && !context.middleMouseDown && !context.rightMouseDown) {
				context.passTask();
			} else if (context.taskIndex == 1 && event.mouseEventType == MouseEventType::MouseDown) {
				if (event.key == MouseKeyEnum::Left) {
					context.passTask();
				} else {
					// TODO: Say which key was triggered instead and suggest skipping with escape if it can not be found.
				}
			} else if (context.taskIndex == 2 && event.mouseEventType == MouseEventType::MouseMove && context.leftMouseDown) {
				context.passTask();
			} else if (context.taskIndex == 3 && event.mouseEventType == MouseEventType::MouseUp) {
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
}
