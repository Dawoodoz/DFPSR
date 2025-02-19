
#include "../testTools.h"
#include "../../DFPSR/api/imageAPI.h"
#include "../../DFPSR/api/drawAPI.h"

START_TEST(Draw)
	// Resources
	ImageU8 imageBall = image_fromAscii(
		"< .x>"
		"< .xx. >"
		"<.xxxx.>"
		"<xxxxxx>"
		"<xxxxxx>"
		"<.xxxx.>"
		"< .xx. >"
	);

	{ // 8-bit monochrome drawing
		int black = 0;
		int gray = 127;
		int white = 255;
		ImageU8 imageA = image_create_U8(16, 16);

		// New images begin with all black pixels
		ASSERT_EQUAL(image_maxDifference(imageA, image_fromAscii(
			"< .x>"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
			"<                >"
		)), 0);

		// Filling an image sets all pixels to the new color
		image_fill(imageA, white);
		ASSERT_EQUAL(image_maxDifference(imageA, image_fromAscii(
			"< .x>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
		)), 0);

		// Drawing a gray rectangle near the upper left corner
		draw_rectangle(imageA, IRect(1, 1, 6, 6), gray);
		ASSERT_EQUAL(image_maxDifference(imageA, image_fromAscii(
			"< .x>"
			"<xxxxxxxxxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
		)), 0);

		// Drawing a gray rectangle near the lower right corner
		draw_rectangle(imageA, IRect(9, 9, 6, 6), gray);
		ASSERT_EQUAL(image_maxDifference(imageA, image_fromAscii(
			"< .x>"
			"<xxxxxxxxxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<x......xxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxxxxxxxxx>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxxxxxxxxx>"
		)), 0);

		// Drawing out of bound at the upper right corner, which is safely clipped to only affect pixels within the current image's view
		draw_rectangle(imageA, IRect(7, -11, 20, 20), black);
		draw_rectangle(imageA, IRect(8, -12, 20, 20), white);
		ASSERT_EQUAL(image_maxDifference(imageA, image_fromAscii(
			"< .x>"
			"<xxxxxxx xxxxxxxx>"
			"<x...... xxxxxxxx>"
			"<x...... xxxxxxxx>"
			"<x...... xxxxxxxx>"
			"<x...... xxxxxxxx>"
			"<x...... xxxxxxxx>"
			"<x...... xxxxxxxx>"
			"<xxxxxxx xxxxxxxx>"
			"<xxxxxxx         >"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxxxxxxxxx>"
		)), 0);

		// Draw diagonal lines from upper left side to lower right side
		draw_line(imageA, 1, 2, 12, 13, 0);
		draw_line(imageA, 2, 2, 13, 13, 255);
		draw_line(imageA, 3, 2, 14, 13, 0);
		ASSERT_EQUAL(image_maxDifference(imageA, image_fromAscii(
			"< .x>"
			"<xxxxxxx xxxxxxxx>"
			"<x...... xxxxxxxx>"
			"<x x ... xxxxxxxx>"
			"<x. x .. xxxxxxxx>"
			"<x.. x . xxxxxxxx>"
			"<x... x  xxxxxxxx>"
			"<x.... x xxxxxxxx>"
			"<xxxxxx x xxxxxxx>"
			"<xxxxxxx x       >"
			"<xxxxxxxx x ....x>"
			"<xxxxxxxxx x ...x>"
			"<xxxxxxxxx. x ..x>"
			"<xxxxxxxxx.. x .x>"
			"<xxxxxxxxx... x x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxxxxxxxxx>"
		)), 0);

		draw_copy(imageA, imageBall, 4, 2);
		ASSERT_EQUAL(image_maxDifference(imageA, image_fromAscii(
			"< .x>"
			"<xxxxxxx xxxxxxxx>"
			"<x...... xxxxxxxx>"
			"<x x  .xx. xxxxxx>"
			"<x. x.xxxx.xxxxxx>"
			"<x.. xxxxxxxxxxxx>"
			"<x...xxxxxxxxxxxx>"
			"<x....xxxx.xxxxxx>"
			"<xxxx .xx. xxxxxx>"
			"<xxxxxxx x       >"
			"<xxxxxxxx x ....x>"
			"<xxxxxxxxx x ...x>"
			"<xxxxxxxxx. x ..x>"
			"<xxxxxxxxx.. x .x>"
			"<xxxxxxxxx... x x>"
			"<xxxxxxxxx......x>"
			"<xxxxxxxxxxxxxxxx>"
		)), 0);
	}
	{ // RGBA silhouette drawing (Giving color to grayscale images by treating silhouette luma as source opacity and the uniform color as source RGB)
		ImageRgbaU8 imageA = image_create_RgbaU8(8, 8);
		draw_rectangle(imageA, IRect(4, 0, 4, 8), ColorRgbaI32(255, 255, 255, 255));
		//printText("imageA R:\n", image_toAscii(image_get_red(imageA), " ,.-x"), "\n");
		//printText("imageA G:\n", image_toAscii(image_get_green(imageA), " ,.-x"), "\n");
		//printText("imageA B:\n", image_toAscii(image_get_blue(imageA), " ,.-x"), "\n");
		ASSERT_LESSER_OR_EQUAL(image_maxDifference(image_get_red(imageA), image_fromAscii(
			"< ,.-x>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
		)), 0);
		ASSERT_LESSER_OR_EQUAL(image_maxDifference(image_get_green(imageA), image_fromAscii(
			"< ,.-x>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
		)), 0);
		ASSERT_LESSER_OR_EQUAL(image_maxDifference(image_get_blue(imageA), image_fromAscii(
			"< ,.-x>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
			"<    xxxx>"
		)), 0);
		// Draw a fully opaque orange ball
		draw_silhouette(imageA, imageBall, ColorRgbaI32(255, 127, 0, 255), 1, 1);
		//printText("imageA R:\n", image_toAscii(image_get_red(imageA), " ,.-x"), "\n");
		//printText("imageA G:\n", image_toAscii(image_get_green(imageA), " ,.-x"), "\n");
		//printText("imageA B:\n", image_toAscii(image_get_blue(imageA), " ,.-x"), "\n");
		ASSERT_LESSER_OR_EQUAL(image_maxDifference(image_get_red(imageA), image_fromAscii(
			"< ,.-x>"
			"<    xxxx>"
			"<  .xxxxx>"
			"< .xxxxxx>"
			"< xxxxxxx>"
			"< xxxxxxx>"
			"< .xxxxxx>"
			"<  .xxxxx>"
			"<    xxxx>"
		)), 1);
		ASSERT_LESSER_OR_EQUAL(image_maxDifference(image_get_green(imageA), image_fromAscii(
			"< ,.-x>"
			"<    xxxx>"
			"<  ,..-xx>"
			"< ,....-x>"
			"< ......x>"
			"< ......x>"
			"< ,....-x>"
			"<  ,..-xx>"
			"<    xxxx>"
		)), 1);
		ASSERT_LESSER_OR_EQUAL(image_maxDifference(image_get_blue(imageA), image_fromAscii(
			"< ,.-x>"
			"<    xxxx>"
			"<     .xx>"
			"<      .x>"
			"<       x>"
			"<       x>"
			"<      .x>"
			"<     .xx>"
			"<    xxxx>"
		)), 1);
		// Draw a half opaque blue ball in the lower right corner
		draw_silhouette(imageA, imageBall, ColorRgbaI32(0, 0, 255, 127), 3, 3);
		//printText("imageA R:\n", image_toAscii(image_get_red(imageA)), "\n");
		//printText("imageA G:\n", image_toAscii(image_get_green(imageA)), "\n");
		//printText("imageA B:\n", image_toAscii(image_get_blue(imageA)), "\n");
		ASSERT_LESSER_OR_EQUAL(image_maxDifference(image_get_red(imageA), image_fromAscii(
			"< .,-_':;!+~=^?*abcdefghijklmnopqrstuvwxyz()[]{}|&@#0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ>"
			"<    ZZZZ>"
			"<  [ZZZZZ>"
			"< [ZZZZZZ>"
			"< ZZZE[[E>"
			"< ZZE[[[[>"
			"< [Z[[[[[>"
			"<  [[[[[[>"
			"<    [[[[>"
		)), 2);
		ASSERT_LESSER_OR_EQUAL(image_maxDifference(image_get_green(imageA), image_fromAscii(
			"< .,-_':;!+~=^?*abcdefghijklmnopqrstuvwxyz()[]{}|&@#0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ>"
			"<    ZZZZ>"
			"<  g[[DZZ>"
			"< g[[[[DZ>"
			"< [[[rhhE>"
			"< [[rhhh[>"
			"< g[hhhr[>"
			"<  ghhr[[>"
			"<    [[[[>"
		)), 2);
		ASSERT_LESSER_OR_EQUAL(image_maxDifference(image_get_blue(imageA), image_fromAscii(
			"< .,-_':;!+~=^?*abcdefghijklmnopqrstuvwxyz()[]{}|&@#0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ>"
			"<    ZZZZ>"
			"<     [ZZ>"
			"<      [Z>"
			"<    g[[Z>"
			"<   g[[[Z>"
			"<   [[[DZ>"
			"<   [[DZZ>"
			"<   gZZZZ>"
		)), 2);
	}

END_TEST

