
#include "../testTools.h"

START_TEST(Pixel)
	{ // ImageU8
		ImageU8 image;
		// Writing to a null image is always ignored
		image_writePixel(image, 0, 0, 137);
		image_writePixel(image, -37, 564, 84);
		// Reading from a null image always returns zero, even if a border color is given
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, 0), 0);
		ASSERT_EQUAL(image_readPixel_clamp(image, -2, 68), 0);
		ASSERT_EQUAL(image_readPixel_tile(image, 0, 0), 0);
		ASSERT_EQUAL(image_readPixel_tile(image, 43, 213), 0);
		ASSERT_EQUAL(image_readPixel_border(image, 0, 0, 26), 0);
		ASSERT_EQUAL(image_readPixel_border(image, -36, -58, 26), 0);
		// Creating a 2x2 image
		image = image_create_U8(2, 2);
		// Saturated writes
		image_writePixel(image, 0, 0, -1);
		image_writePixel(image, 1, 0, 256);
		image_writePixel(image, 0, 1, -56456);
		image_writePixel(image, 1, 1, 76546);
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, 0), 0);
		ASSERT_EQUAL(image_readPixel_clamp(image, 1, 0), 255);
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, 1), 0);
		ASSERT_EQUAL(image_readPixel_clamp(image, 1, 1), 255);
		// Writing pixels
		image_writePixel(image, 0, 0, 12);
		image_writePixel(image, 1, 0, 34);
		image_writePixel(image, 0, 1, 56);
		image_writePixel(image, 1, 1, 78);
		// Writing pixels out of bound is also ignored
		image_writePixel(image, -1, 0, 45);
		image_writePixel(image, 1, 2, 15);
		image_writePixel(image, -463, 175, 245);
		image_writePixel(image, 987463, -75563, 64);
		// Sample inside
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, 0), 12);
		ASSERT_EQUAL(image_readPixel_clamp(image, 1, 0), 34);
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, 1), 56);
		ASSERT_EQUAL(image_readPixel_clamp(image, 1, 1), 78);
		ASSERT_EQUAL(image_readPixel_tile(image, 0, 0), 12);
		ASSERT_EQUAL(image_readPixel_tile(image, 1, 0), 34);
		ASSERT_EQUAL(image_readPixel_tile(image, 0, 1), 56);
		ASSERT_EQUAL(image_readPixel_tile(image, 1, 1), 78);
		ASSERT_EQUAL(image_readPixel_border(image, 0, 0, 23), 12);
		ASSERT_EQUAL(image_readPixel_border(image, 1, 0, 75), 34);
		ASSERT_EQUAL(image_readPixel_border(image, 0, 1, 34), 56);
		ASSERT_EQUAL(image_readPixel_border(image, 1, 1, 21), 78);
		// Sample outside
		ASSERT_EQUAL(image_readPixel_clamp(image, -3, 0), 12);
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, -1), 12);
		ASSERT_EQUAL(image_readPixel_clamp(image, 4, 0), 34);
		ASSERT_EQUAL(image_readPixel_clamp(image, 1, -1), 34);
		ASSERT_EQUAL(image_readPixel_clamp(image, -4, 1), 56);
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, 2), 56);
		ASSERT_EQUAL(image_readPixel_clamp(image, 2, 1), 78);
		ASSERT_EQUAL(image_readPixel_clamp(image, 1, 5), 78);
		ASSERT_EQUAL(image_readPixel_clamp(image, -24, -63), 12);
		ASSERT_EQUAL(image_readPixel_clamp(image, 37, -45), 34);
		ASSERT_EQUAL(image_readPixel_clamp(image, -1, 2), 56);
		ASSERT_EQUAL(image_readPixel_clamp(image, 34, 5), 78);
		// Borders are returned as is without saturation, which can be used for unique error codes
		ASSERT_EQUAL(image_readPixel_border(image, -23854, -61456, -23), -23);
		ASSERT_EQUAL(image_readPixel_border(image, 7564, 254, 376), 376);
		ASSERT_EQUAL(image_readPixel_border(image, -1457, 734166, 3), 3);
		ASSERT_EQUAL(image_readPixel_border(image, 62489, -17350, 1245), 1245);
		ASSERT_EQUAL(image_readPixel_border(image, 0, -1, 128), 128);
		ASSERT_EQUAL(image_readPixel_border(image, 1, -1, 498), 498);
		ASSERT_EQUAL(image_readPixel_border(image, 0, 2, -25), -25);
		ASSERT_EQUAL(image_readPixel_border(image, 1, 2, 47), 47);
	}
	{ // ImageF32
		ImageF32 image;
		// Writing to a null image is always ignored
		image_writePixel(image, 0, 0, 137.0f);
		image_writePixel(image, -37, 564, 84.0f);
		// Reading from a null image always returns zero, even if a border color is given
		ASSERT_NEAR(image_readPixel_clamp(image, 0, 0), 0.0f);
		ASSERT_NEAR(image_readPixel_clamp(image, -2, 68), 0.0f);
		ASSERT_NEAR(image_readPixel_tile(image, 0, 0), 0.0f);
		ASSERT_NEAR(image_readPixel_tile(image, 43, 213), 0.0f);
		ASSERT_NEAR(image_readPixel_border(image, 0, 0, 26.0f), 0.0f);
		ASSERT_NEAR(image_readPixel_border(image, -36, -58, 26.0f), 0.0f);
		// Creating a 2x2 image
		image = image_create_F32(2, 2);
		// Writing pixels
		image_writePixel(image, 0, 0, 12.3f);
		image_writePixel(image, 1, 0, 23.4f);
		image_writePixel(image, 0, 1, 34.5f);
		image_writePixel(image, 1, 1, 45.6f);
		// Writing pixels out of bound is also ignored
		image_writePixel(image, -1, 0, 45.652f);
		image_writePixel(image, 1, 2, 15.459f);
		image_writePixel(image, -463, 175, 245.516f);
		image_writePixel(image, 987463, -75563, 64.342f);
		// Sample inside
		ASSERT_NEAR(image_readPixel_clamp(image, 0, 0), 12.3f);
		ASSERT_NEAR(image_readPixel_clamp(image, 1, 0), 23.4f);
		ASSERT_NEAR(image_readPixel_clamp(image, 0, 1), 34.5f);
		ASSERT_NEAR(image_readPixel_clamp(image, 1, 1), 45.6f);
		ASSERT_NEAR(image_readPixel_tile(image, 0, 0), 12.3f);
		ASSERT_NEAR(image_readPixel_tile(image, 1, 0), 23.4f);
		ASSERT_NEAR(image_readPixel_tile(image, 0, 1), 34.5f);
		ASSERT_NEAR(image_readPixel_tile(image, 1, 1), 45.6f);
		ASSERT_NEAR(image_readPixel_border(image, 0, 0, 23.53f), 12.3f);
		ASSERT_NEAR(image_readPixel_border(image, 1, 0, 75.16f), 23.4f);
		ASSERT_NEAR(image_readPixel_border(image, 0, 1, 23.48f), 34.5f);
		ASSERT_NEAR(image_readPixel_border(image, 1, 1, 21.64f), 45.6f);
		// Sample outside
		ASSERT_NEAR(image_readPixel_clamp(image, -3, 0), 12.3f);
		ASSERT_NEAR(image_readPixel_clamp(image, 0, -1), 12.3f);
		ASSERT_NEAR(image_readPixel_clamp(image, 4, 0), 23.4f);
		ASSERT_NEAR(image_readPixel_clamp(image, 1, -1), 23.4f);
		ASSERT_NEAR(image_readPixel_clamp(image, -4, 1), 34.5f);
		ASSERT_NEAR(image_readPixel_clamp(image, 0, 2), 34.5f);
		ASSERT_NEAR(image_readPixel_clamp(image, 2, 1), 45.6f);
		ASSERT_NEAR(image_readPixel_clamp(image, 1, 5), 45.6f);
		ASSERT_NEAR(image_readPixel_clamp(image, -24, -63), 12.3f);
		ASSERT_NEAR(image_readPixel_clamp(image, 37, -45), 23.4f);
		ASSERT_NEAR(image_readPixel_clamp(image, -1, 2), 34.5f);
		ASSERT_NEAR(image_readPixel_clamp(image, 34, 5), 45.6f);
		// Borders are returned as is, because float doesn't require saturation
		ASSERT_NEAR(image_readPixel_border(image, -23854, -61456, -23.4f), -23.4f);
		ASSERT_NEAR(image_readPixel_border(image, 7564, 254, 376.8f), 376.8f);
		ASSERT_NEAR(image_readPixel_border(image, -1457, 734166, 3.0f), 3.0f);
		ASSERT_NEAR(image_readPixel_border(image, 62489, -17350, 1245.2f), 1245.2f);
		ASSERT_NEAR(image_readPixel_border(image, 0, -1, 128.0f), 128.0f);
		ASSERT_NEAR(image_readPixel_border(image, 1, -1, 498.4f), 498.4f);
		ASSERT_NEAR(image_readPixel_border(image, 0, 2, -25.9f), -25.9f);
		ASSERT_NEAR(image_readPixel_border(image, 1, 2, 47.1f), 47.1f);
	}
	{ // ImageRgbaU8
		ImageRgbaU8 image;
		// Writing to a null image is always ignored
		image_writePixel(image, 0, 0, ColorRgbaI32(25, 73, 8, 43));
		image_writePixel(image, -37, 564, ColorRgbaI32(86, 45, 68, 14));
		// Reading from a null image always returns zero, even if a border color is given
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, 0), ColorRgbaI32(0, 0, 0, 0));
		ASSERT_EQUAL(image_readPixel_clamp(image, -2, 68), ColorRgbaI32(0, 0, 0, 0));
		ASSERT_EQUAL(image_readPixel_tile(image, 0, 0), ColorRgbaI32(0, 0, 0, 0));
		ASSERT_EQUAL(image_readPixel_tile(image, 43, 213), ColorRgbaI32(0, 0, 0, 0));
		ASSERT_EQUAL(image_readPixel_border(image, 0, 0, ColorRgbaI32(65, 96, 135, 57)), ColorRgbaI32(0, 0, 0, 0));
		ASSERT_EQUAL(image_readPixel_border(image, -36, -58, ColorRgbaI32(12, 75, 58, 53)), ColorRgbaI32(0, 0, 0, 0));
		// Creating a 2x2 image
		image = image_create_RgbaU8(2, 2);
		// Writing pixels with saturation
		image_writePixel(image, 0, 0, ColorRgbaI32(-36, 7645, -75, 345)); // Saturated to (0, 255, 0, 255)
		image_writePixel(image, 1, 0, ColorRgbaI32(1000, 477, 684, 255)); // Saturated to (255, 255, 255, 255)
		image_writePixel(image, 0, 1, ColorRgbaI32(-1, 0, 255, 256));     // Saturated to (0, 0, 255, 255)
		image_writePixel(image, 1, 1, ColorRgbaI32(0, 25, 176, 255));     // No effect from saturation
		// Writing pixels out of bound is also ignored
		image_writePixel(image, -1, 0, ColorRgbaI32(-57, 486, 65, 377));
		image_writePixel(image, 1, 2, ColorRgbaI32(7, 4, 6, 84));
		image_writePixel(image, -463, 175, ColorRgbaI32(86, 0, 47, 255));
		image_writePixel(image, 987463, -75563, ColorRgbaI32(55, 86, 55, 123));
		// Sample inside
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, 0), ColorRgbaI32(0, 255, 0, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, 1, 0), ColorRgbaI32(255, 255, 255, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, 1), ColorRgbaI32(0, 0, 255, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, 1, 1), ColorRgbaI32(0, 25, 176, 255));
		ASSERT_EQUAL(image_readPixel_tile(image, 0, 0), ColorRgbaI32(0, 255, 0, 255));
		ASSERT_EQUAL(image_readPixel_tile(image, 1, 0), ColorRgbaI32(255, 255, 255, 255));
		ASSERT_EQUAL(image_readPixel_tile(image, 0, 1), ColorRgbaI32(0, 0, 255, 255));
		ASSERT_EQUAL(image_readPixel_tile(image, 1, 1), ColorRgbaI32(0, 25, 176, 255));
		ASSERT_EQUAL(image_readPixel_border(image, 0, 0, ColorRgbaI32(54, 37, 66, 36)), ColorRgbaI32(0, 255, 0, 255));
		ASSERT_EQUAL(image_readPixel_border(image, 1, 0, ColorRgbaI32(12, 75, 58, 47)), ColorRgbaI32(255, 255, 255, 255));
		ASSERT_EQUAL(image_readPixel_border(image, 0, 1, ColorRgbaI32(75, 68, 72, 44)), ColorRgbaI32(0, 0, 255, 255));
		ASSERT_EQUAL(image_readPixel_border(image, 1, 1, ColorRgbaI32(86, 45, 77, 34)), ColorRgbaI32(0, 25, 176, 255));
		// Sample outside
		ASSERT_EQUAL(image_readPixel_clamp(image, -3, 0), ColorRgbaI32(0, 255, 0, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, -1), ColorRgbaI32(0, 255, 0, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, 4, 0), ColorRgbaI32(255, 255, 255, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, 1, -1), ColorRgbaI32(255, 255, 255, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, -4, 1), ColorRgbaI32(0, 0, 255, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, 0, 2), ColorRgbaI32(0, 0, 255, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, 2, 1), ColorRgbaI32(0, 25, 176, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, 1, 5), ColorRgbaI32(0, 25, 176, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, -24, -63), ColorRgbaI32(0, 255, 0, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, 37, -45), ColorRgbaI32(255, 255, 255, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, -1, 2), ColorRgbaI32(0, 0, 255, 255));
		ASSERT_EQUAL(image_readPixel_clamp(image, 34, 5), ColorRgbaI32(0, 25, 176, 255));
		// Borders are returned as is without saturation, which can be used for unique error codes
		ASSERT_EQUAL(image_readPixel_border(image, -23854, -61456, ColorRgbaI32(-1, -1, -1, -1)), ColorRgbaI32(-1, -1, -1, -1));
		ASSERT_EQUAL(image_readPixel_border(image, 7564, 254, ColorRgbaI32(1245, 84, -215, 43)), ColorRgbaI32(1245, 84, -215, 43));
		ASSERT_EQUAL(image_readPixel_border(image, -1457, 734166, ColorRgbaI32(2000, 5, 2, 7)), ColorRgbaI32(2000, 5, 2, 7));
		ASSERT_EQUAL(image_readPixel_border(image, 62489, -17350, ColorRgbaI32(253, 46, 1574, 64)), ColorRgbaI32(253, 46, 1574, 64));
		ASSERT_EQUAL(image_readPixel_border(image, 0, -1, ColorRgbaI32(0, 0, 0, -1)), ColorRgbaI32(0, 0, 0, -1));
		ASSERT_EQUAL(image_readPixel_border(image, 1, -1, ColorRgbaI32(99, 99, 99, 99)), ColorRgbaI32(99, 99, 99, 99));
		ASSERT_EQUAL(image_readPixel_border(image, 0, 2, ColorRgbaI32(1, 2, 3, 4)), ColorRgbaI32(1, 2, 3, 4));
		ASSERT_EQUAL(image_readPixel_border(image, 1, 2, ColorRgbaI32(-1, -2, -3, -4)), ColorRgbaI32(-1, -2, -3, -4));
	}
END_TEST

