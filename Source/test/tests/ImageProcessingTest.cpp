
#include "../testTools.h"
#include "../../DFPSR/api/imageAPI.h"
#include "../../DFPSR/api/filterAPI.h"
#include "../../DFPSR/base/simd.h"

AlignedImageU8 addImages_generate(ImageU8 imageA, ImageU8 imageB) {
	int width = image_getWidth(imageA);
	int height = image_getHeight(imageA);
	// Call your lambda for width times height pixels
	return filter_generateU8(width, height,
		[imageA, imageB](int x, int y) {
			int lumaA = image_readPixel_clamp(imageA, x, y);
			int lumaB = image_readPixel_clamp(imageB, x, y);
			return lumaA + lumaB;
		}
	);
}

void addImages_map(ImageU8 targetImage, ImageU8 imageA, ImageU8 imageB) {
	// Call your lambda for each pixel in targetImage
	filter_mapU8(targetImage,
		[imageA, imageB](int x, int y) {
			int lumaA = image_readPixel_clamp(imageA, x, y);
			int lumaB = image_readPixel_clamp(imageB, x, y);
			return lumaA + lumaB;
		}
	);
}

void addImages_loop(ImageU8 targetImage, ImageU8 imageA, ImageU8 imageB) {
	int width = image_getWidth(targetImage);
	int height = image_getHeight(targetImage);
	// Loop over all x, y coordinates yourself
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int lumaA = image_readPixel_clamp(imageA, x, y);
			int lumaB = image_readPixel_clamp(imageB, x, y);
			image_writePixel(targetImage, x, y, lumaA + lumaB);
		}
	}
}

void addImages_pointer(ImageU8 targetImage, ImageU8 imageA, ImageU8 imageB) {
	int width = image_getWidth(targetImage);
	int height = image_getHeight(targetImage);
	SafePointer<uint8_t> targetRow = image_getSafePointer(targetImage);
	SafePointer<uint8_t> rowA = image_getSafePointer(imageA);
	SafePointer<uint8_t> rowB = image_getSafePointer(imageB);
	int targetStride = image_getStride(targetImage);
	int strideA = image_getStride(imageA);
	int strideB = image_getStride(imageB);
	for (int y = 0; y < height; y++) {
		SafePointer<uint8_t> targetPixel = targetRow;
		SafePointer<uint8_t> pixelA = rowA;
		SafePointer<uint8_t> pixelB = rowB;
		for (int x = 0; x < width; x++) {
			// Read both source pixels and add them
			int result = *pixelA + *pixelB;
			// Clamp overflow
			if (result > 255) result = 255;
			// Can skip underflow check
			//if (result < 0) result = 0;
			// Write the result
			*targetPixel = result;
			// Move pixel pointers to the next pixel
			targetPixel += 1;
			pixelA += 1;
			pixelB += 1;
		}
		// Move row pointers to the next row
		targetRow.increaseBytes(targetStride);
		rowA.increaseBytes(strideA);
		rowB.increaseBytes(strideB);
	}
}

void addImages_simd(AlignedImageU8 targetImage, AlignedImageU8 imageA, AlignedImageU8 imageB) {
	int width = image_getWidth(targetImage);
	int height = image_getHeight(targetImage);
	SafePointer<uint8_t> targetRow = image_getSafePointer(targetImage);
	SafePointer<uint8_t> rowA = image_getSafePointer(imageA);
	SafePointer<uint8_t> rowB = image_getSafePointer(imageB);
	int targetStride = image_getStride(targetImage);
	int strideA = image_getStride(imageA);
	int strideB = image_getStride(imageB);
	for (int y = 0; y < height; y++) {
		SafePointer<uint8_t> targetPixel = targetRow;
		SafePointer<uint8_t> pixelA = rowA;
		SafePointer<uint8_t> pixelB = rowB;
		// Assuming that we have ownership of any padding pixels
		for (int x = 0; x < width; x += 16) {
			// Read 16 source pixels at a time
			U8x16 a = U8x16::readAligned(pixelA, "addImages: reading pixelA");
			U8x16 b = U8x16::readAligned(pixelB, "addImages: reading pixelB");
			// Saturated operations replace conditional move
			U8x16 result = saturatedAddition(a, b);
			// Write the result 16 pixels at a time
			result.writeAligned(targetPixel, "addImages: writing result");
			// Move pixel pointers to the next pixel
			targetPixel += 16;
			pixelA += 16;
			pixelB += 16;
		}
		// Move row pointers to the next row
		targetRow.increaseBytes(targetStride);
		rowA.increaseBytes(strideA);
		rowB.increaseBytes(strideB);
	}
}

// Making sure that all code examples from the image processing guide actually works
START_TEST(ImageProcessing)
	{
		AlignedImageU8 imageA = image_fromAscii(
			"< .x>"
			"<         ...   >"
			"<        .xx.   >"
			"<    .....      >"
			"<     .xx..     >"
			"<  ..x..        >"
			"<    ......     >"
			"<        ..xx.. >"
			"<     ..x.      >"
			"<      ..x..    >"
			"<       ..x.    >"
			"<      ...      >"
			"<        ...    >"
			"<       ...     >"
			"<      .x..     >"
		);
		AlignedImageU8 imageB = image_fromAscii(
			"< .x>"
			"<               >"
			"<               >"
			"<               >"
			"<..             >"
			"<.xx...         >"
			"<...xxxx....    >"
			"<   ...xxxxxx...>"
			"<      ....xxxxx>"
			"<           ...x>"
			"<              .>"
			"<..             >"
			"<x....          >"
			"<xxx...         >"
			"<xx..           >"
		);
		// Using the generate method as a reference implementation.
		AlignedImageU8 imageExpected = addImages_generate(imageA, imageB);
		//printText("\nGenerate result:\n", image_toAscii(imageExpected));
		
		AlignedImageU8 imageResult = image_create_U8(15, 14);
		addImages_map(imageResult, imageA, imageB);
		//printText("\nMap result:\n", image_toAscii(imageResult));
		ASSERT_EQUAL(image_maxDifference(imageResult, imageExpected), 0);

		addImages_loop(imageResult, imageA, imageB);
		//printText("\nLoop result:\n", image_toAscii(imageResult));
		ASSERT_EQUAL(image_maxDifference(imageResult, imageExpected), 0);

		addImages_pointer(imageResult, imageA, imageB);
		//printText("\nPointer result:\n", image_toAscii(imageResult));
		ASSERT_EQUAL(image_maxDifference(imageResult, imageExpected), 0);

		addImages_simd(imageResult, imageA, imageB);
		//printText("\nSIMD result:\n", image_toAscii(imageResult));
		ASSERT_EQUAL(image_maxDifference(imageResult, imageExpected), 0);
	}
END_TEST
