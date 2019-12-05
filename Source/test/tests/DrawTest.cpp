
#include "../testTools.h"

START_TEST(Draw)
	ImageU8 imageA = image_create_U8(16, 16);
	uint8_t black = 0;
	uint8_t gray = 127;
	uint8_t white = 255;
	ImageU8 expected1 = image_fromAscii(
		"< .x>"
		"<xxxxxxxxxxxxxxxx>"
		"<x......xx      x>"
		"<x......xx      x>"
		"<x..xx..xx  xx  x>"
		"<x..xx..xx  xx  x>"
		"<x......xx      x>"
		"<x......xx      x>"
		"<xxxxxxxxxxxxxxxx>"
		"<xxxxxxxxxxxxxxxx>"
		"<x      xx......x>"
		"<x      xx......x>"
		"<x  xx  xx..xx..x>"
		"<x  xx  xx..xx..x>"
		"<x      xx......x>"
		"<x      xx......x>"
		"<xxxxxxxxxxxxxxxx>"
	);

	// TODO: Try drawing outside without crashing. Only drawing to a broken target image should allow crashing.

	// Fill the image
	image_fill(imageA, white);
	draw_rectangle(imageA, IRect(1, 1, 6, 6), gray);
	draw_rectangle(imageA, IRect(9, 9, 6, 6), gray);
	draw_rectangle(imageA, IRect(9, 1, 6, 6), black);
	draw_rectangle(imageA, IRect(1, 9, 6, 6), black);
	draw_rectangle(imageA, IRect(3, 3, 2, 2), white);
	draw_rectangle(imageA, IRect(3, 11, 2, 2), white);
	draw_rectangle(imageA, IRect(11, 3, 2, 2), white);
	draw_rectangle(imageA, IRect(11, 11, 2, 2), white);

	// TODO: Make a reusable macro for comparing images and showing them when a test fails.
	ASSERT_LESSER_OR_EQUAL(image_maxDifference(imageA, expected1), 1);
	//printText("imageA:\n", image_toAscii(imageA, " .x"), "\n");
END_TEST

