// zlib open source license
//
// Copyright (c) 2019 David Forsgren Piuva
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
#include "../base/simd.h"

using namespace dsr;

template <typename T, typename U>
static void assertSameSize(const T& imageL, const U& imageR) {
	if (!image_exists(imageL) || !image_exists(imageR)) {
		if (image_exists(imageL)) {
			// Left side exists, so there's no right side
			throwError("Media filter: Non-existing right side input image.\n");
		} else if (image_exists(imageR)) {
			// Right side exists, so there's no left side
			throwError("Media filter: Non-existing left side input image.\n");
		} else {
			// Neither input exists
			throwError("Media filter: Non-existing input images.\n");
		}
	} else if (image_getWidth(imageL) != image_getWidth(imageR)
	       || image_getHeight(imageL) != image_getHeight(imageR)) {
		throwError("Media filter: Taking input images of different dimensions, ", image_getWidth(imageL), "x", image_getHeight(imageL), " and ", image_getWidth(imageR), "x", image_getHeight(imageR), ".\n");
	}
}

template <typename T>
static void assertExisting(const T& image) {
	if (!image_exists(image)) {
		throwError("Media filter: Non-existing input image.\n");
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
			throwError("Media filter: Cannot allocate to size of non-existing input image.\n");
		}
		targetImage = image_create_U8(image_getWidth(inputImage), image_getHeight(inputImage));
	}
}

void dsr::media_filter_add(AlignedImageU8& targetImage, AlignedImageU8 imageL, AlignedImageU8 imageR) {
	assertSameSize(imageL, imageR);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, imageL);
	// TODO: Implement U8x16 in simd.h
	//       readAligned, writeAligned, addSaturated, subtractSaturated...
	/*for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		const SafePointer<uint8_t> targetRow = imageInternal::getSafeData<uint8_t>(targetImage, y);
		const SafePointer<uint8_t> sourceRowL = imageInternal::getSafeData<uint8_t>(imageL, y);
		const SafePointer<uint8_t> sourceRowR = imageInternal::getSafeData<uint8_t>(imageR, y);
		for (int32_t x = 0; x < image_getWidth(targetImage); x += 4) {
			ALIGN16 U8x16 colorL = U8x16::readAligned(sourceRowL, "media_filter_add (sourceRowL)");
			ALIGN16 U8x16 colorR = U8x16::readAligned(sourceRowR, "media_filter_add (sourceRowR)");
			ALIGN16 U8x16 result = U8x16::addSaturated(colorL, colorR);
			result.writeAligned(targetRow, "media_filter_add (targetRow)");
			targetRow += 16;
			sourceRowL += 16;
			sourceRowR += 16;
		}
	}*/
	// Reference implementation
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		for (int32_t x = 0; x < image_getWidth(targetImage); x++) {
			image_writePixel(targetImage, x, y, image_readPixel_clamp(imageL, x, y) + image_readPixel_clamp(imageR, x, y));
		}
	}
}

void dsr::media_filter_add(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint scalar) {
	assertExisting(image);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, image);
	// Reference implementation
	int whole = fixedPoint_round(scalar);
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		for (int32_t x = 0; x < image_getWidth(targetImage); x++) {
			image_writePixel(targetImage, x, y, image_readPixel_clamp(image, x, y) + whole);
		}
	}
}

void dsr::media_filter_sub(AlignedImageU8& targetImage, AlignedImageU8 imageL, AlignedImageU8 imageR) {
	assertSameSize(imageL, imageR);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, imageL);
	// Reference implementation
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		for (int32_t x = 0; x < image_getWidth(targetImage); x++) {
			image_writePixel(targetImage, x, y, image_readPixel_clamp(imageL, x, y) - image_readPixel_clamp(imageR, x, y));
		}
	}
}

void dsr::media_filter_sub(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint scalar) {
	assertExisting(image);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, image);
	// Reference implementation
	int whole = fixedPoint_round(scalar);
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		for (int32_t x = 0; x < image_getWidth(targetImage); x++) {
			image_writePixel(targetImage, x, y, image_readPixel_clamp(image, x, y) - whole);
		}
	}
}

void dsr::media_filter_sub(AlignedImageU8& targetImage, FixedPoint scalar, AlignedImageU8 image) {
	assertExisting(image);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, image);
	// Reference implementation
	int whole = fixedPoint_round(scalar);
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		for (int32_t x = 0; x < image_getWidth(targetImage); x++) {
			image_writePixel(targetImage, x, y, whole - image_readPixel_clamp(image, x, y));
		}
	}
}

void dsr::media_filter_mul(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint scalar) {
	assertExisting(image);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, image);
	// Reference implementation
	int64_t mantissa = scalar.getMantissa();
	if (mantissa < 0) { mantissa = 0; } // At least zero, because negative clamps to zero
	if (mantissa > 16711680) { mantissa = 16711680; } // At most 255 whole integers, became more makes no difference
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		for (int32_t x = 0; x < image_getWidth(targetImage); x++) {
			image_writePixel(targetImage, x, y, ((int64_t)image_readPixel_clamp(image, x, y) * mantissa) / 65536);
		}
	}
}

void dsr::media_filter_mul(AlignedImageU8& targetImage, AlignedImageU8 imageL, AlignedImageU8 imageR, FixedPoint scalar) {
	assertSameSize(imageL, imageR);
	removeIfShared(targetImage);
	allocateToSameSize(targetImage, imageL);
	// Reference implementation
	int64_t mantissa = scalar.getMantissa();
	if (mantissa < 0) { mantissa = 0; } // At least zero, because negative clamps to zero
	if (mantissa > 16711680) { mantissa = 16711680; } // At most 255 whole integers, became more makes no difference
	for (int32_t y = 0; y < image_getHeight(targetImage); y++) {
		for (int32_t x = 0; x < image_getWidth(targetImage); x++) {
			int32_t result = ((uint64_t)image_readPixel_clamp(imageL, x, y) * (uint64_t)image_readPixel_clamp(imageR, x, y) * mantissa) / 65536;
			image_writePixel(targetImage, x, y, result);
		}
	}
}

void dsr::media_fade_region_linear(ImageU8& targetImage, const IRect& viewport, FixedPoint x1, FixedPoint y1, FixedPoint luma1, FixedPoint x2, FixedPoint y2, FixedPoint luma2) {
	assertExisting(targetImage);
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
	// TODO: Optimize the cases where ratioDx == 0 (memset per line) or ratioDy == 0 (memcpy from first line)
	for (int32_t y = viewport.top(); y < viewport.bottom(); y++) {
		int64_t ratio = startRatio;
		for (int32_t x = viewport.left(); x < viewport.right(); x++) {
			int64_t saturatedRatio = ratio;
			// TODO: Reuse this code section
			if (saturatedRatio < 0) { saturatedRatio = 0; }
			if (saturatedRatio > 65536) { saturatedRatio = 65536; }
			int64_t mixedColor = ((luma1.getMantissa() * (65536 - ratio)) + (luma2.getMantissa() * ratio) + 2147483648ll) / 4294967296ll;
			if (mixedColor < 0) { mixedColor = 0; }
			if (mixedColor > 255) { mixedColor = 255; }
			// TODO: Write the already saturated result using safe pointers to the target image
			image_writePixel(targetImage, x, y, mixedColor);
			ratio += ratioDx;
		}
		startRatio += ratioDy;
	}
}

void dsr::media_fade_linear(ImageU8& targetImage, FixedPoint x1, FixedPoint y1, FixedPoint luma1, FixedPoint x2, FixedPoint y2, FixedPoint luma2) {
	media_fade_region_linear(targetImage, image_getBound(targetImage), x1, y1, luma1, x2, y2, luma2);
}

void dsr::media_fade_region_radial(ImageU8& targetImage, const IRect& viewport, FixedPoint centerX, FixedPoint centerY, FixedPoint innerRadius, FixedPoint innerLuma, FixedPoint outerRadius, FixedPoint outerLuma) {
	assertExisting(targetImage);
	if (innerLuma < 0) { innerLuma = FixedPoint::zero(); }
	if (innerLuma > 255) { innerLuma = FixedPoint::fromWhole(255); }
	if (outerLuma < 0) { outerLuma = FixedPoint::zero(); }
	if (outerLuma > 255) { outerLuma = FixedPoint::fromWhole(255); }
	// Subtracting half a pixel in the fade line is equivalent to adding half a pixel on X and Y
	FixedPoint originX = centerX + viewport.left() - FixedPoint::half();
	FixedPoint originY = centerY + viewport.top() - FixedPoint::half();
	// Let outerRadius be slightly outside of innerRadius to prevent division by zero
	if (outerRadius <= innerRadius) {
		outerRadius = innerRadius + FixedPoint::epsilon();
	}
	FixedPoint reciprocalFadeLength = FixedPoint::one() / (outerRadius - innerRadius);
	for (int32_t y = viewport.top(); y < viewport.bottom(); y++) {
		for (int32_t x = viewport.left(); x < viewport.right(); x++) {
			FixedPoint diffX = x - originX;
			FixedPoint diffY = y - originY;
			FixedPoint length = fixedPoint_squareRoot((diffX * diffX) + (diffY * diffY));
			FixedPoint ratio = (length - innerRadius) * reciprocalFadeLength;
			int64_t saturatedRatio = ratio.getMantissa();
			// TODO: Reuse this code section
			if (saturatedRatio < 0) { saturatedRatio = 0; }
			if (saturatedRatio > 65536) { saturatedRatio = 65536; }
			int64_t mixedColor = ((innerLuma.getMantissa() * (65536 - ratio.getMantissa())) + (outerLuma.getMantissa() * ratio.getMantissa()) + 2147483648ll) / 4294967296ll;
			if (mixedColor < 0) { mixedColor = 0; }
			if (mixedColor > 255) { mixedColor = 255; }
			// TODO: Write the already saturated result using safe pointers to the target image
			image_writePixel(targetImage, x, y, mixedColor);
		}
	}
}

void dsr::media_fade_radial(ImageU8& targetImage, FixedPoint centerX, FixedPoint centerY, FixedPoint innerRadius, FixedPoint innerLuma, FixedPoint outerRadius, FixedPoint outerLuma) {
	media_fade_region_radial(targetImage, image_getBound(targetImage), centerX, centerY, innerRadius, innerLuma, outerRadius, outerLuma);
}
