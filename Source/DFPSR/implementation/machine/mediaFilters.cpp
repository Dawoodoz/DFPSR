// zlib open source license
//
// Copyright (c) 2019 to 2022 David Forsgren Piuva
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
//    2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 
//    3. This notice may not be removed or altered from any source
//    distribution.

#include "mediaFilters.h"
#include "../../base/simd.h"
#include "../../api/drawAPI.h"

using namespace dsr;

template <typename T, typename U>
static void assertSameSize(const T& imageA, const U& imageB) {
	if (!image_exists(imageA) || !image_exists(imageB)) {
		if (image_exists(imageA)) {
			// Left side exists, so there's no right side
			throwError(U"Media filter: Non-existing right side input image.\n");
		} else if (image_exists(imageB)) {
			// Right side exists, so there's no left side
			throwError(U"Media filter: Non-existing left side input image.\n");
		} else {
			// Neither input exists
			throwError(U"Media filter: Non-existing input images.\n");
		}
	} else if (image_getWidth(imageA) != image_getWidth(imageB)
	       || image_getHeight(imageA) != image_getHeight(imageB)) {
		throwError(U"Media filter: Taking input images of different dimensions, ", image_getWidth(imageA), U"x", image_getHeight(imageA), U" and ", image_getWidth(imageB), U"x", image_getHeight(imageB), U".\n");
	}
}

template <typename T>
static void assertExisting(const T& image) {
	if (!image_exists(image)) {
		throwError(U"Media filter: Non-existing input image.\n");
	}
}

template <typename T>
static void removeIfShared(T& targetImage) {
	if (image_useCount(targetImage) > 1) {
		targetImage = AlignedImageU8();
	}
}

template <typename T, typename U>
static void allocateToSameSize(T& targetImage, const U& inputImage) {
	if (!image_exists(targetImage) || image_getWidth(targetImage) != image_getWidth(inputImage) || image_getHeight(targetImage) != image_getHeight(inputImage)) {
		if (!image_exists(inputImage)) {
			throwError(U"Media filter: Cannot allocate to size of non-existing input image.\n");
		}
		targetImage = image_create_U8(image_getWidth(inputImage), image_getHeight(inputImage));
	}
}

void dsr::media_filter_add(AlignedImageU8& targetImage, AlignedImageU8 imageA, AlignedImageU8 imageB) {
	assertSameSize(imageA, imageB);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, imageA);
	SafePointer<uint8_t> targetRow = image_getSafePointer(targetImage);
	SafePointer<uint8_t> sourceRowA = image_getSafePointer(imageA);
	SafePointer<uint8_t> sourceRowB = image_getSafePointer(imageB);
	int32_t targetStride = image_getStride(targetImage);
	int32_t sourceStrideA = image_getStride(imageA);
	int32_t sourceStrideB = image_getStride(imageB);
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		SafePointer<uint8_t> targetPixel = targetRow;
		SafePointer<uint8_t> sourcePixelA = sourceRowA;
		SafePointer<uint8_t> sourcePixelB = sourceRowB;
		for (int32_t x = 0; x < image_getWidth(targetImage); x += 16) {
			U8x16 colorA = U8x16::readAligned(sourcePixelA, "media_filter_add (sourcePixelA)");
			U8x16 colorB = U8x16::readAligned(sourcePixelB, "media_filter_add (sourcePixelB)");
			U8x16 result = saturatedAddition(colorA, colorB);
			result.writeAligned(targetPixel, "media_filter_add (targetPixel)");
			targetPixel += 16;
			sourcePixelA += 16;
			sourcePixelB += 16;
		}
		targetRow.increaseBytes(targetStride);
		sourceRowA.increaseBytes(sourceStrideA);
		sourceRowB.increaseBytes(sourceStrideB);
	}
}

void dsr::media_filter_add(AlignedImageU8& targetImage, AlignedImageU8 image, int32_t luma) {
	assertExisting(image);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, image);
	if (luma < 0) luma = 0;
	if (luma > 255) luma = 255;
	SafePointer<uint8_t> targetRow = image_getSafePointer(targetImage);
	SafePointer<uint8_t> sourceRowA = image_getSafePointer(image);
	int32_t targetStride = image_getStride(targetImage);
	int32_t sourceStride = image_getStride(image);
	U8x16 repeatedLuma = U8x16(luma);
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		SafePointer<uint8_t> targetPixel = targetRow;
		SafePointer<uint8_t> sourcePixel = sourceRowA;
		for (int32_t x = 0; x < image_getWidth(targetImage); x += 16) {
			U8x16 colorA = U8x16::readAligned(sourcePixel, "media_filter_add (sourcePixel)");
			U8x16 result = saturatedAddition(colorA, repeatedLuma);
			result.writeAligned(targetPixel, "media_filter_add (targetPixel)");
			targetPixel += 16;
			sourcePixel += 16;
		}
		targetRow.increaseBytes(targetStride);
		sourceRowA.increaseBytes(sourceStride);
	}
}

void dsr::media_filter_add(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint luma) {
	media_filter_add(targetImage, image, fixedPoint_round(luma));
}

void dsr::media_filter_sub(AlignedImageU8& targetImage, AlignedImageU8 imageA, AlignedImageU8 imageB) {
	assertSameSize(imageA, imageB);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, imageA);
	SafePointer<uint8_t> targetRow = image_getSafePointer(targetImage);
	SafePointer<uint8_t> sourceRowA = image_getSafePointer(imageA);
	SafePointer<uint8_t> sourceRowB = image_getSafePointer(imageB);
	int32_t targetStride = image_getStride(targetImage);
	int32_t sourceStrideA = image_getStride(imageA);
	int32_t sourceStrideB = image_getStride(imageB);
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		SafePointer<uint8_t> targetPixel = targetRow;
		SafePointer<uint8_t> sourcePixelA = sourceRowA;
		SafePointer<uint8_t> sourcePixelB = sourceRowB;
		for (int32_t x = 0; x < image_getWidth(targetImage); x += 16) {
			U8x16 colorA = U8x16::readAligned(sourcePixelA, "media_filter_sub (sourcePixelA)");
			U8x16 colorB = U8x16::readAligned(sourcePixelB, "media_filter_sub (sourcePixelB)");
			U8x16 result = saturatedSubtraction(colorA, colorB);
			result.writeAligned(targetPixel, "media_filter_sub (targetPixel)");
			targetPixel += 16;
			sourcePixelA += 16;
			sourcePixelB += 16;
		}
		targetRow.increaseBytes(targetStride);
		sourceRowA.increaseBytes(sourceStrideA);
		sourceRowB.increaseBytes(sourceStrideB);
	}
}

void dsr::media_filter_sub(AlignedImageU8& targetImage, AlignedImageU8 image, int32_t luma) {
	assertExisting(image);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, image);
	if (luma < 0) luma = 0;
	if (luma > 255) luma = 255;
	SafePointer<uint8_t> targetRow = image_getSafePointer(targetImage);
	SafePointer<uint8_t> sourceRowA = image_getSafePointer(image);
	int32_t targetStride = image_getStride(targetImage);
	int32_t sourceStride = image_getStride(image);
	U8x16 repeatedLuma = U8x16(luma);
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		SafePointer<uint8_t> targetPixel = targetRow;
		SafePointer<uint8_t> sourcePixel = sourceRowA;
		for (int32_t x = 0; x < image_getWidth(targetImage); x += 16) {
			U8x16 colorA = U8x16::readAligned(sourcePixel, "media_filter_sub (sourcePixel)");
			U8x16 result = saturatedSubtraction(colorA, repeatedLuma);
			result.writeAligned(targetPixel, "media_filter_sub (targetPixel)");
			targetPixel += 16;
			sourcePixel += 16;
		}
		targetRow.increaseBytes(targetStride);
		sourceRowA.increaseBytes(sourceStride);
	}
}

void dsr::media_filter_sub(AlignedImageU8& targetImage, int32_t luma, AlignedImageU8 image) {
	assertExisting(image);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, image);
	if (luma < 0) luma = 0;
	if (luma > 255) luma = 255;
	SafePointer<uint8_t> targetRow = image_getSafePointer(targetImage);
	SafePointer<uint8_t> sourceRowA = image_getSafePointer(image);
	int32_t targetStride = image_getStride(targetImage);
	int32_t sourceStride = image_getStride(image);
	U8x16 repeatedLuma = U8x16(luma);
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		SafePointer<uint8_t> targetPixel = targetRow;
		SafePointer<uint8_t> sourcePixel = sourceRowA;
		for (int32_t x = 0; x < image_getWidth(targetImage); x += 16) {
			U8x16 colorA = U8x16::readAligned(sourcePixel, "media_filter_sub (sourcePixel)");
			U8x16 result = saturatedSubtraction(repeatedLuma, colorA);
			result.writeAligned(targetPixel, "media_filter_sub (targetPixel)");
			targetPixel += 16;
			sourcePixel += 16;
		}
		targetRow.increaseBytes(targetStride);
		sourceRowA.increaseBytes(sourceStride);
	}
}

void dsr::media_filter_sub(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint luma) {
	media_filter_sub(targetImage, image, fixedPoint_round(luma));
}

void dsr::media_filter_sub(AlignedImageU8& targetImage, FixedPoint luma, AlignedImageU8 image) {
	media_filter_sub(targetImage, fixedPoint_round(luma), image);
}

void dsr::media_filter_mul(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint luma) {
	assertExisting(image);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, image);
	// Reference implementation
	int64_t mantissa = luma.getMantissa();
	if (mantissa < 0) { mantissa = 0; } // At least zero, because negative clamps to zero
	if (mantissa > 16711680) { mantissa = 16711680; } // At most 255 whole integers, became more makes no difference
	SafePointer<uint8_t> targetRow = image_getSafePointer(targetImage);
	SafePointer<uint8_t> sourceRow = image_getSafePointer(image);
	int32_t targetStride = image_getStride(targetImage);
	int32_t sourceStride = image_getStride(image);
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		SafePointer<uint8_t> targetPixel = targetRow;
		SafePointer<uint8_t> sourcePixel = sourceRow;
		for (int32_t x = 0; x < image_getWidth(targetImage); x++) {
			int64_t result = ((int64_t)(*sourcePixel) * mantissa) / 65536;
			if (result < 0) { result = 0; }
			if (result > 255) { result = 255; }
			*targetPixel = result;
			targetPixel += 1;
			sourcePixel += 1;
		}
		targetRow.increaseBytes(targetStride);
		sourceRow.increaseBytes(sourceStride);
	}
}

void dsr::media_filter_mul(AlignedImageU8& targetImage, AlignedImageU8 imageA, AlignedImageU8 imageB, FixedPoint luma) {
	assertSameSize(imageA, imageB);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, imageA);
	// Reference implementation
	int64_t mantissa = luma.getMantissa();
	if (mantissa < 0) { mantissa = 0; } // At least zero, because negative clamps to zero
	if (mantissa > 16711680) { mantissa = 16711680; } // At most 255 whole integers, became more makes no difference
	SafePointer<uint8_t> targetRow = image_getSafePointer(targetImage);
	SafePointer<uint8_t> sourceRowA = image_getSafePointer(imageA);
	SafePointer<uint8_t> sourceRowB = image_getSafePointer(imageB);
	int32_t targetStride = image_getStride(targetImage);
	int32_t sourceStrideA = image_getStride(imageA);
	int32_t sourceStrideB = image_getStride(imageB);
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		SafePointer<uint8_t> targetPixel = targetRow;
		SafePointer<uint8_t> sourcePixelA = sourceRowA;
		SafePointer<uint8_t> sourcePixelB = sourceRowB;
		for (int32_t x = 0; x < image_getWidth(targetImage); x++) {
			int64_t result = (((uint64_t)*sourcePixelA) * ((uint64_t)*sourcePixelB) * mantissa) / 65536;
			if (result < 0) { result = 0; }
			if (result > 255) { result = 255; }
			*targetPixel = result;
			targetPixel += 1;
			sourcePixelA += 1;
			sourcePixelB += 1;
		}
		targetRow.increaseBytes(targetStride);
		sourceRowA.increaseBytes(sourceStrideA);
		sourceRowB.increaseBytes(sourceStrideB);
	}
}

void dsr::media_fade_region_linear(ImageU8& targetImage, const IRect& viewport, FixedPoint x1, FixedPoint y1, FixedPoint luma1, FixedPoint x2, FixedPoint y2, FixedPoint luma2) {
	assertExisting(targetImage);
	IRect safeBound = IRect::cut(viewport, image_getBound(targetImage));
	// Saturate luma in advance
	if (luma1 < 0) { luma1 = FixedPoint::zero(); }
	if (luma1 > 255) { luma1 = FixedPoint::fromWhole(255); }
	if (luma2 < 0) { luma2 = FixedPoint::zero(); }
	if (luma2 > 255) { luma2 = FixedPoint::fromWhole(255); }
	// Subtracting half a pixel in the fade line is equivalent to adding half a pixel on X and Y during sampling
	int64_t startX = x1.getMantissa() - 32768;
	int64_t startY = y1.getMantissa() - 32768;
	int64_t endX = x2.getMantissa() - 32768;
	int64_t endY = y2.getMantissa() - 32768;
	int64_t diffX = endX - startX; // x2 - x1 * 65536
	int64_t diffY = endY - startY; // y2 - y1 * 65536
	// You don't need to get the linear lengths nor distance.
	//   By both generating a squared length and using a dot product, no square root is required.
	//   This is because length(v)² = dot(v, v)
	int64_t squareLength = ((diffX * diffX) + (diffY * diffY)) / 65536; // length² * 65536
	// Limit to at least one pixel's length, both to get anti-aliasing and prevent overflow.
	if (squareLength < 65536) { squareLength = 65536; }
	// Calculate ratios for 3 pixels using dot products
	int64_t offsetX = -startX; // First pixel relative to x1
	int64_t offsetY = -startY; // First pixel relative to y1
	int64_t offsetX_right = 65536 - startX; // Right pixel relative to x1
	int64_t offsetY_down = 65536 - startY; // Down pixel relative to y1
	int64_t dotProduct = ((offsetX * diffX) + (offsetY * diffY)) / 65536; // dot(offset, diff) * 65536
	int64_t dotProduct_right = ((offsetX_right * diffX) + (offsetY * diffY)) / 65536; // dot(offsetRight, diff) * 65536
	int64_t dotProduct_down = ((offsetX * diffX) + (offsetY_down * diffY)) / 65536; // dot(offsetDown, diff) * 65536
	int64_t startRatio = (dotProduct * 65536 / squareLength); // The color mix ratio at the first pixel in a scale from 0 to 65536
	int64_t ratioDx = (dotProduct_right * 65536 / squareLength) - startRatio; // The color mix difference when going right
	int64_t ratioDy = (dotProduct_down * 65536 / squareLength) - startRatio; // The color mix difference when going down
	SafePointer<uint8_t> targetRow = image_getSafePointer(targetImage, safeBound.top()) + safeBound.left();
	int32_t targetStride = image_getStride(targetImage);
	if (ratioDx == 0) {
		if (ratioDy == 0) {
			// No direction at all. Fill the whole rectangle with luma1.
			draw_rectangle(targetImage, safeBound, fixedPoint_round(luma1));
		} else {
			// Up or down using memset per row.
			int32_t widthInBytes = safeBound.width();
			for (int32_t y = safeBound.top(); y < safeBound.bottom(); y++) {
				int64_t saturatedRatio = startRatio;
				if (saturatedRatio < 0) { saturatedRatio = 0; }
				if (saturatedRatio > 65536) { saturatedRatio = 65536; }
				int64_t mixedColor = ((luma1.getMantissa() * (65536 - saturatedRatio)) + (luma2.getMantissa() * saturatedRatio) + 2147483648ll) / 4294967296ll;
				if (mixedColor < 0) { mixedColor = 0; }
				if (mixedColor > 255) { mixedColor = 255; }
				safeMemorySet<uint8_t>(targetRow, mixedColor, widthInBytes);
				targetRow.increaseBytes(targetStride);
				startRatio += ratioDy;
			}
		}
	} else {
		if (ratioDy == 0) {
			// Left or right using memcpy per row.
			SafePointer<uint8_t> sourceRow = targetRow;
			SafePointer<uint8_t> targetPixel = targetRow;
			int64_t ratio = startRatio;
			int32_t widthInBytes = safeBound.width();
			// Evaluate the first line.
			for (int32_t x = viewport.left(); x < viewport.right(); x++) {
				int64_t saturatedRatio = ratio;
				if (saturatedRatio < 0) { saturatedRatio = 0; }
				if (saturatedRatio > 65536) { saturatedRatio = 65536; }
				int64_t mixedColor = ((luma1.getMantissa() * (65536 - saturatedRatio)) + (luma2.getMantissa() * saturatedRatio) + 2147483648ll) / 4294967296ll;
				if (mixedColor < 0) { mixedColor = 0; }
				if (mixedColor > 255) { mixedColor = 255; }
				*targetPixel = mixedColor;
				targetPixel += 1;
				ratio += ratioDx;
			}
			// Copy the rest from the first line.
			for (int32_t y = viewport.top() + 1; y < viewport.bottom(); y++) {
				targetRow.increaseBytes(targetStride);
				safeMemoryCopy<uint8_t>(targetRow, sourceRow, widthInBytes);
			}
		} else {
			// Each pixel needs to be evaluated in this fade.
			for (int32_t y = viewport.top(); y < viewport.bottom(); y++) {
				SafePointer<uint8_t> targetPixel = targetRow;
				for (int32_t x = viewport.left(); x < viewport.right(); x++) {
				int64_t saturatedRatio = startRatio;
				if (saturatedRatio < 0) { saturatedRatio = 0; }
				if (saturatedRatio > 65536) { saturatedRatio = 65536; }
				int64_t mixedColor = ((luma1.getMantissa() * (65536 - saturatedRatio)) + (luma2.getMantissa() * saturatedRatio) + 2147483648ll) / 4294967296ll;
					if (mixedColor < 0) { mixedColor = 0; }
					if (mixedColor > 255) { mixedColor = 255; }
					*targetPixel = mixedColor;
					targetPixel += 1;
				}
				targetRow.increaseBytes(targetStride);
				startRatio += ratioDy;
			}
		}
	}
}

void dsr::media_fade_linear(ImageU8& targetImage, FixedPoint x1, FixedPoint y1, FixedPoint luma1, FixedPoint x2, FixedPoint y2, FixedPoint luma2) {
	media_fade_region_linear(targetImage, image_getBound(targetImage), x1, y1, luma1, x2, y2, luma2);
}

void dsr::media_fade_region_radial(ImageU8& targetImage, const IRect& viewport, FixedPoint centerX, FixedPoint centerY, FixedPoint innerRadius, FixedPoint innerLuma, FixedPoint outerRadius, FixedPoint outerLuma) {
	assertExisting(targetImage);
	IRect safeBound = IRect::cut(viewport, image_getBound(targetImage));
	if (innerLuma < 0) { innerLuma = FixedPoint::zero(); }
	if (innerLuma > 255) { innerLuma = FixedPoint::fromWhole(255); }
	if (outerLuma < 0) { outerLuma = FixedPoint::zero(); }
	if (outerLuma > 255) { outerLuma = FixedPoint::fromWhole(255); }
	// Subtracting half a pixel in the fade line is equivalent to adding half a pixel on X and Y.
	int64_t originX = centerX.getMantissa() + safeBound.left() * 65536 - 32768;
	int64_t originY = centerY.getMantissa() + safeBound.top() * 65536 - 32768;
	// Let outerRadius be slightly outside of innerRadius to prevent division by zero and get anti-aliasing.
	if (outerRadius <= innerRadius + FixedPoint::one()) {
		outerRadius = innerRadius + FixedPoint::one();
	}
	int64_t fadeSize = (outerRadius.getMantissa() - innerRadius.getMantissa());
	int64_t fadeSlope = 4294967296ll / fadeSize;
	SafePointer<uint8_t> targetRow = image_getSafePointer(targetImage, safeBound.top()) + safeBound.left();
	int32_t targetStride = image_getStride(targetImage);
	for (int64_t y = safeBound.top(); y < safeBound.bottom(); y++) {
		SafePointer<uint8_t> targetPixel = targetRow;
		for (int64_t x = safeBound.left(); x < safeBound.right(); x++) {
			int64_t diffX = (x * 65536) - originX;
			int64_t diffY = (y * 65536) - originY;
			// Double's square root is guaranteed to be exact for integers fitting inside of its mantissa.
			int64_t length = sqrt(((diffX * diffX) + (diffY * diffY)));
			// Using a 64-bit integer division per pixel for good quality and high range.
			int64_t ratio = ((length - innerRadius.getMantissa()) * fadeSlope) / 65536;
			int64_t saturatedRatio = ratio;
			if (saturatedRatio < 0) { saturatedRatio = 0; }
			if (saturatedRatio > 65536) { saturatedRatio = 65536; }
			int64_t mixedColor = ((innerLuma.getMantissa() * (65536 - saturatedRatio)) + (outerLuma.getMantissa() * saturatedRatio) + 2147483648ll) / 4294967296ll;
			if (mixedColor < 0) { mixedColor = 0; }
			if (mixedColor > 255) { mixedColor = 255; }
			*targetPixel = mixedColor;
			targetPixel += 1;
		}
		targetRow.increaseBytes(targetStride);
	}
}

void dsr::media_fade_radial(ImageU8& targetImage, FixedPoint centerX, FixedPoint centerY, FixedPoint innerRadius, FixedPoint innerLuma, FixedPoint outerRadius, FixedPoint outerLuma) {
	media_fade_region_radial(targetImage, image_getBound(targetImage), centerX, centerY, innerRadius, innerLuma, outerRadius, outerLuma);
}
