
#include "../testTools.h"

START_TEST(Image)
	{ // ImageU8
		ImageU8 imageA;
		ASSERT_EQUAL(image_exists(imageA), false);
		imageA = image_create_U8(17, 9);
		ASSERT_EQUAL(image_exists(imageA), true);
		ASSERT_EQUAL(image_useCount(imageA), 1);
		ASSERT_EQUAL(image_getWidth(imageA), 17);
		ASSERT_EQUAL(image_getHeight(imageA), 9);
		ASSERT_EQUAL(image_getStride(imageA), heap_getHeapAlignment());
		ASSERT_EQUAL(image_getBound(imageA), IRect(0, 0, 17, 9));
		ImageU8 imageB; // Create empty image reference
		ASSERT_EQUAL(image_useCount(imageA), 1);
		ASSERT_EQUAL(image_useCount(imageB), 0);
		imageB = imageA; // Shallow copy of reference
		ASSERT_EQUAL(image_useCount(imageA), 2);
		ASSERT_EQUAL(image_useCount(imageB), 2);
		imageA = ImageU8(); // Remove original reference to the image
		ASSERT_EQUAL(image_useCount(imageA), 0);
		ASSERT_EQUAL(image_useCount(imageB), 1);
	}
	{ // ImageF32
		ImageF32 image;
		ASSERT_EQUAL(image_exists(image), false);
		image = image_create_F32(3, 48);
		ASSERT_EQUAL(image_exists(image), true);
		ASSERT_EQUAL(image_useCount(image), 1);
		ASSERT_EQUAL(image_getWidth(image), 3);
		ASSERT_EQUAL(image_getHeight(image), 48);
		ASSERT_EQUAL(image_getStride(image), heap_getHeapAlignment());
		ASSERT_EQUAL(image_getBound(image), IRect(0, 0, 3, 48));
	}
	{ // ImageRgbaU8
		ImageRgbaU8 image;
		ASSERT_EQUAL(image_exists(image), false);
		image = image_create_RgbaU8(52, 12);
		ASSERT_EQUAL(image_exists(image), true);
		ASSERT_EQUAL(image_useCount(image), 1);
		ASSERT_EQUAL(image_getWidth(image), 52);
		ASSERT_EQUAL(image_getHeight(image), 12);
		ASSERT_EQUAL(image_getBound(image), IRect(0, 0, 52, 12));
	}
	{ // Sub-images
		ImageU8 parentImage = image_fromAscii(
			"< .x>"
			"< ..  .. >"
			"<..x..x..>"
			"<.xx..xx.>"
			"< ..xx.. >"
			"< ..xx.. >"
			"<.xx..xx.>"
			"<..x..x..>"
			"< ..  .. >"
		);
		ImageU8 upperLeftSubImage = image_getSubImage(parentImage, IRect(0, 0, 4, 4));
		ImageU8 upperRightSubImage = image_getSubImage(parentImage, IRect(4, 0, 4, 4));
		ImageU8 lowerLeftSubImage = image_getSubImage(parentImage, IRect(0, 4, 4, 4));
		ImageU8 lowerRightSubImage = image_getSubImage(parentImage, IRect(4, 4, 4, 4));
		ImageU8 centerSubImage = image_getSubImage(parentImage, IRect(2, 2, 4, 4));
		ASSERT_EQUAL(image_maxDifference(upperLeftSubImage, image_fromAscii(
			"< .x>"
			"< .. >"
			"<..x.>"
			"<.xx.>"
			"< ..x>"
		)), 0);
		ASSERT_EQUAL(image_maxDifference(upperRightSubImage, image_fromAscii(
			"< .x>"
			"< .. >"
			"<.x..>"
			"<.xx.>"
			"<x.. >"
		)), 0);
		ASSERT_EQUAL(image_maxDifference(lowerLeftSubImage, image_fromAscii(
			"< .x>"
			"< ..x>"
			"<.xx.>"
			"<..x.>"
			"< .. >"
		)), 0);
		ASSERT_EQUAL(image_maxDifference(lowerRightSubImage, image_fromAscii(
			"< .x>"
			"<x.. >"
			"<.xx.>"
			"<.x..>"
			"< .. >"
		)), 0);
		ASSERT_EQUAL(image_maxDifference(centerSubImage, image_fromAscii(
			"< .x>"
			"<x..x>"
			"<.xx.>"
			"<.xx.>"
			"<x..x>"
		)), 0);
		image_fill(centerSubImage, 0);
		ASSERT_EQUAL(image_maxDifference(parentImage, image_fromAscii(
			"< .x>"
			"< ..  .. >"
			"<..x..x..>"
			"<.x    x.>"
			"< .    . >"
			"< .    . >"
			"<.x    x.>"
			"<..x..x..>"
			"< ..  .. >"
		)), 0);
	}
END_TEST

