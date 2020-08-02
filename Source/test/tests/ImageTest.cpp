
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
		ASSERT_EQUAL(image_getStride(imageA), 32);
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
		ASSERT_EQUAL(image_getStride(image), 16);
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
		ASSERT_EQUAL(image_getStride(image), 208);
		ASSERT_EQUAL(image_getBound(image), IRect(0, 0, 52, 12));
	}
	{ // RGBA Texture
		ImageRgbaU8 image;
		image = image_create_RgbaU8(256, 256);
		ASSERT_EQUAL(image_hasPyramid(image), false);
		image_generatePyramid(image);
		ASSERT_EQUAL(image_hasPyramid(image), true);
		image_removePyramid(image);
		ASSERT_EQUAL(image_hasPyramid(image), false);
		image_generatePyramid(image);
		ASSERT_EQUAL(image_hasPyramid(image), true);
	}
END_TEST

