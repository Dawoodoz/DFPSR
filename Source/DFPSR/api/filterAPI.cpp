
// zlib open source license
//
// Copyright (c) 2017 to 2025 David Forsgren Piuva
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

// TODO: Optimize and clean up using template programming to automatically unpack image data in advance for easy access.
//       Create reusable inline functions for fast pixel sampling in a separate header while prototyping.

#include <cassert>
#include "filterAPI.h"
#include "imageAPI.h"
#include "drawAPI.h"
#include "../image/PackOrder.h"
#include "../base/simd.h"

namespace dsr {

static inline U32x4 ColorRgbaI32_to_U32x4(const ColorRgbaI32& color) {
	return U32x4(color.red, color.green, color.blue, color.alpha);
}

static inline ColorRgbaI32 U32x4_to_ColorRgbaI32(const U32x4& color) {
	UVector4D vResult = color.get();
	return ColorRgbaI32(vResult.x, vResult.y, vResult.z, vResult.w);
}

// Uniform linear interpolation of colors from a 16-bit sub-pixel weight
// Pre-condition0 <= fineRatio <= 65536
// Post-condition: Returns colorA * (1 - (fineRatio / 65536)) + colorB * (fineRatio / 65536)
static inline U32x4 mixColorsUniform(const U32x4 &colorA, const U32x4 &colorB, uint32_t fineRatio) {
	uint16_t ratio = (uint16_t)bitShiftRightImmediate<8>(fineRatio);
	uint16_t invRatio = 256 - ratio;
	U16x8 weightA = U16x8(invRatio);
	U16x8 weightB = U16x8(ratio);
	U32x4 lowMask(0x00FF00FFu);
	U16x8 lowColorA = U16x8(colorA & lowMask);
	U16x8 lowColorB = U16x8(colorB & lowMask);
	U32x4 highMask(0xFF00FF00u);
	U16x8 highColorA = bitShiftRightImmediate<8>(U16x8((colorA & highMask)));
	U16x8 highColorB = bitShiftRightImmediate<8>(U16x8((colorB & highMask)));
	U32x4 lowColor = (((lowColorA * weightA) + (lowColorB * weightB))).get_U32();
	U32x4 highColor = (((highColorA * weightA) + (highColorB * weightB))).get_U32();
	return ((bitShiftRightImmediate<8>(lowColor) & lowMask) | (highColor & highMask));
}

// TODO: Use wrappers around images to get the needed information unpacked in advance for faster reading of pixels.
#define READ_RGBAU8_CLAMP(X,Y) image_readPixel_clamp(source, X, Y)
#define READ_RGBAU8_CLAMP_SIMD(X,Y) ColorRgbaI32_to_U32x4(READ_RGBAU8_CLAMP(X,Y))

// Fixed-precision decimal system with 16-bit indices and 16-bit sub-pixel weights
static const uint32_t interpolationFullPixel = 65536;
static const uint32_t interpolationHalfPixel = interpolationFullPixel / 2;
// Modulo mask for values greater than or equal to 0 and lesser than interpolationFullPixel
static const uint32_t interpolationWeightMask = interpolationFullPixel - 1;

template <bool BILINEAR>
static uint32_t samplePixel(const ImageRgbaU8& target, const ImageRgbaU8& source, uint32_t leftX, uint32_t upperY, uint32_t rightRatio, uint32_t lowerRatio) {
	if (BILINEAR) {
		uint32_t upperRatio = 65536 - lowerRatio;
		uint32_t leftRatio = 65536 - rightRatio;
		U32x4 vUpperLeftColor = READ_RGBAU8_CLAMP_SIMD(leftX, upperY);
		U32x4 vUpperRightColor = READ_RGBAU8_CLAMP_SIMD(leftX + 1, upperY);
		U32x4 vLowerLeftColor = READ_RGBAU8_CLAMP_SIMD(leftX, upperY + 1);
		U32x4 vLowerRightColor = READ_RGBAU8_CLAMP_SIMD(leftX + 1, upperY + 1);
		U32x4 vLeftRatio = U32x4(leftRatio);
		U32x4 vRightRatio = U32x4(rightRatio);
		U32x4 vUpperColor = bitShiftRightImmediate<16>((vUpperLeftColor * vLeftRatio) + (vUpperRightColor * vRightRatio));
		U32x4 vLowerColor = bitShiftRightImmediate<16>((vLowerLeftColor * vLeftRatio) + (vLowerRightColor * vRightRatio));
		U32x4 vCenterColor = bitShiftRightImmediate<16>((vUpperColor * upperRatio) + (vLowerColor * lowerRatio));
		return image_saturateAndPack(target, U32x4_to_ColorRgbaI32(vCenterColor));
	} else {
		return image_saturateAndPack(target, image_readPixel_clamp(source, leftX, upperY));
	}
}

template <bool BILINEAR>
static uint8_t samplePixel(const ImageU8& target, const ImageU8& source, uint32_t leftX, uint32_t upperY, uint32_t rightRatio, uint32_t lowerRatio) {
	if (BILINEAR) {
		uint32_t upperRatio = 65536 - lowerRatio;
		uint32_t leftRatio = 65536 - rightRatio;
		uint32_t upperLeftLuma = image_readPixel_clamp(source, leftX, upperY);
		uint32_t upperRightLuma = image_readPixel_clamp(source, leftX + 1, upperY);
		uint32_t lowerLeftLuma = image_readPixel_clamp(source, leftX, upperY + 1);
		uint32_t lowerRightLuma = image_readPixel_clamp(source, leftX + 1, upperY + 1);
		uint32_t upperLuma = bitShiftRightImmediate<16>((upperLeftLuma * leftRatio) + (upperRightLuma * rightRatio));
		uint32_t lowerLuma = bitShiftRightImmediate<16>((lowerLeftLuma * leftRatio) + (lowerRightLuma * rightRatio));
		return bitShiftRightImmediate<16>((upperLuma * upperRatio) + (lowerLuma * lowerRatio));
	} else {
		return image_readPixel_clamp(source, leftX, upperY);
	}
}

// BILINEAR: Enables linear interpolation
// scaleRegion:
//     The stretched location of the source image in the target image
//     Making it smaller than the target image will fill the outside with stretched pixels
//     Allowing the caller to crop away parts of the source image that aren't interesting
//     Can be used to round the region to a multiple of the input size for a fixed pixel size
template <bool BILINEAR, typename IMAGE_TYPE, typename PIXEL_TYPE>
static void resize_reference(const IMAGE_TYPE& target, const IMAGE_TYPE& source, const IRect& scaleRegion) {
	// Reference implementation

	// Offset in source pixels per target pixel
	int32_t offsetX = interpolationFullPixel * image_getWidth(source) / scaleRegion.width();
	int32_t offsetY = interpolationFullPixel * image_getHeight(source) / scaleRegion.height();
	int32_t startX = interpolationFullPixel * scaleRegion.left() + offsetX / 2;
	int32_t startY = interpolationFullPixel * scaleRegion.top() + offsetY / 2;
	if (BILINEAR) {
		startX -= interpolationHalfPixel;
		startY -= interpolationHalfPixel;
	}
	SafePointer<PIXEL_TYPE> targetRow = image_getSafePointer<PIXEL_TYPE>(target);
	int32_t readY = startY;
	for (int32_t y = 0; y < image_getHeight(target); y++) {
		int32_t naturalY = readY;
		if (naturalY < 0) { naturalY = 0; }
		uint32_t sampleY = (uint32_t)naturalY;
		uint32_t upperY = bitShiftRightImmediate<16>(sampleY);
		uint32_t lowerRatio = sampleY & interpolationWeightMask;
		SafePointer<PIXEL_TYPE> targetPixel = targetRow;
		int32_t readX = startX;
		for (int32_t x = 0; x < image_getWidth(target); x++) {
			int32_t naturalX = readX;
			if (naturalX < 0) { naturalX = 0; }
			uint32_t sampleX = (uint32_t)naturalX;
			uint32_t leftX = bitShiftRightImmediate<16>(sampleX);
			uint32_t rightRatio = sampleX & interpolationWeightMask;
			*targetPixel = samplePixel<BILINEAR>(target, source, leftX, upperY, rightRatio, lowerRatio);
			targetPixel += 1;
			readX += offsetX;
		}
		targetRow.increaseBytes(image_getStride(target));
		readY += offsetY;
	}
}

template <bool BILINEAR, bool SIMD_ALIGNED>
static void resize_optimized(const ImageRgbaU8& target, const ImageRgbaU8& source, const IRect& scaleRegion) {
	// Get source information
	// Compare dimensions
	const bool sameWidth = image_getWidth(source) == scaleRegion.width() && scaleRegion.left() == 0;
	const bool sameHeight = image_getHeight(source) == scaleRegion.height() && scaleRegion.top() == 0;
	const bool samePackOrder = image_getPackOrderIndex(target) == image_getPackOrderIndex(source);
	if (sameWidth && sameHeight) {
		// No need to resize, just make a copy to save time
		draw_copy(target, source);
	} else if (sameWidth && (samePackOrder || BILINEAR)) {
		// Only vertical interpolation

		// Offset in source pixels per target pixel
		int32_t offsetY = interpolationFullPixel * image_getHeight(source) / scaleRegion.height();
		int32_t startY = interpolationFullPixel * scaleRegion.top() + offsetY / 2;
		if (BILINEAR) {
			startY -= interpolationHalfPixel;
		}
		SafePointer<uint32_t> targetRow = image_getSafePointer<uint32_t>(target);
		int32_t readY = startY;
		for (int32_t y = 0; y < image_getHeight(target); y++) {
			int32_t naturalY = readY;
			if (naturalY < 0) { naturalY = 0; }
			uint32_t sampleY = (uint32_t)naturalY;
			uint32_t upperY = bitShiftRightImmediate<16>(sampleY);
			uint32_t lowerY = upperY + 1;
			if (upperY >= (uint32_t)image_getHeight(source)) upperY = image_getHeight(source) - 1;
			if (lowerY >= (uint32_t)image_getHeight(source)) lowerY = image_getHeight(source) - 1;
			if (BILINEAR) {
				uint32_t lowerRatio = sampleY & interpolationWeightMask;
				uint32_t upperRatio = 65536 - lowerRatio;
				SafePointer<uint32_t> targetPixel = targetRow;
				if (SIMD_ALIGNED) {
					SafePointer<const uint32_t> sourceRowUpper = image_getSafePointer<uint32_t>(source, upperY);
					SafePointer<const uint32_t> sourceRowLower = image_getSafePointer<uint32_t>(source, lowerY);
					for (int32_t x = 0; x < image_getWidth(target); x += 4) {
						ALIGN16 U32x4 vUpperPackedColor = U32x4::readAligned(sourceRowUpper, "resize_optimized @ read vUpperPackedColor");
						ALIGN16 U32x4 vLowerPackedColor = U32x4::readAligned(sourceRowLower, "resize_optimized @ read vLowerPackedColor");
						ALIGN16 U32x4 vCenterColor = mixColorsUniform(vUpperPackedColor, vLowerPackedColor, lowerRatio);
						vCenterColor.writeAligned(targetPixel, "resize_optimized @ write vCenterColor");
						sourceRowUpper += 4;
						sourceRowLower += 4;
						targetPixel += 4;
					}
				} else {
					for (int32_t x = 0; x < image_getWidth(target); x++) {
						ALIGN16 U32x4 vUpperColor = READ_RGBAU8_CLAMP_SIMD(x, upperY);
						ALIGN16 U32x4 vLowerColor = READ_RGBAU8_CLAMP_SIMD(x, lowerY);
						ALIGN16 U32x4 vCenterColor = bitShiftRightImmediate<16>((vUpperColor * upperRatio) + (vLowerColor * lowerRatio));
						ColorRgbaI32 finalColor = U32x4_to_ColorRgbaI32(vCenterColor);
						*targetPixel = image_saturateAndPack(target, finalColor);
						targetPixel += 1;
					}
				}
			} else {
				SafePointer<const uint32_t> sourceRowUpper = image_getSafePointer<uint32_t>(source, upperY);
				// Nearest neighbor sampling from a same width can be done using one copy per row
				safeMemoryCopy(targetRow, sourceRowUpper, image_getWidth(source) * 4);
			}
			targetRow.increaseBytes(image_getStride(target));
			readY += offsetY;
		}
	} else if (sameHeight) {
		// Only horizontal interpolation

		// Offset in source pixels per target pixel
		int32_t offsetX = interpolationFullPixel * image_getWidth(source) / scaleRegion.width();
		int32_t startX = interpolationFullPixel * scaleRegion.left() + offsetX / 2;
		if (BILINEAR) {
			startX -= interpolationHalfPixel;
		}
		SafePointer<uint32_t> targetRow = image_getSafePointer<uint32_t>(target);
		for (int32_t y = 0; y < image_getHeight(target); y++) {
			SafePointer<uint32_t> targetPixel = targetRow;
			int32_t readX = startX;
			for (int32_t x = 0; x < image_getWidth(target); x++) {
				int32_t naturalX = readX;
				if (naturalX < 0) { naturalX = 0; }
				uint32_t sampleX = (uint32_t)naturalX;
				uint32_t leftX = bitShiftRightImmediate<16>(sampleX);
				uint32_t rightX = leftX + 1;
				uint32_t rightRatio = sampleX & interpolationWeightMask;
				uint32_t leftRatio = 65536 - rightRatio;
				ColorRgbaI32 finalColor;
				if (BILINEAR) {
					ALIGN16 U32x4 vLeftColor = READ_RGBAU8_CLAMP_SIMD(leftX, y);
					ALIGN16 U32x4 vRightColor = READ_RGBAU8_CLAMP_SIMD(rightX, y);
					ALIGN16 U32x4 vCenterColor = bitShiftRightImmediate<16>((vLeftColor * leftRatio) + (vRightColor * rightRatio));
					finalColor = U32x4_to_ColorRgbaI32(vCenterColor);
				} else {
					finalColor = READ_RGBAU8_CLAMP(leftX, y);
				}
				*targetPixel = image_saturateAndPack(target, finalColor);
				targetPixel += 1;
				readX += offsetX;
			}
			targetRow.increaseBytes(image_getStride(target));
		}
	} else {
		// Call the reference implementation
		resize_reference<BILINEAR, ImageRgbaU8, uint32_t>(target, source, scaleRegion);
	}
}

// Converting run-time flags into compile-time constants
static void resize_aux(const ImageRgbaU8& target, const ImageRgbaU8& source, bool interpolate, const IRect& scaleRegion) {
	// If writing to padding is allowed and both images are 16-byte aligned with the same pack order
	if (!(image_isSubImage(source) || image_isSubImage(target))) {
		// SIMD resize allowed
		if (interpolate) {
			resize_optimized<true, true>(target, source, scaleRegion);
		} else {
			resize_optimized<false, true>(target, source, scaleRegion);
		}
	} else {
		// Non-SIMD resize
		if (interpolate) {
			resize_optimized<true, false>(target, source, scaleRegion);
		} else {
			resize_optimized<false, false>(target, source, scaleRegion);
		}
	}
}

// TODO: Optimize monochrome resizing.
static void resize_aux(const ImageU8& target, const ImageU8& source, bool interpolate, const IRect& scaleRegion) {
	if (interpolate) {
		resize_reference<true, ImageU8, uint8_t>(target, source, scaleRegion);
	} else {
		resize_reference<false, ImageU8, uint8_t>(target, source, scaleRegion);
	}
}

// Creating an image to replacedImage with the same pack order as originalImage when applicable to the image format.
static ImageRgbaU8 createWithSamePackOrder(const ImageRgbaU8& originalImage, int32_t width, int32_t height) {
	return image_create_RgbaU8_native(width, height, image_getPackOrderIndex(originalImage));
}
static ImageU8 createWithSamePackOrder(const ImageU8& originalImage, int32_t width, int32_t height) {
	return image_create_U8(width, height);
}

template <typename IMAGE_TYPE>
void resizeToTarget(IMAGE_TYPE& target, const IMAGE_TYPE& source, bool interpolate) {
	IRect scaleRegion = image_getBound(target);
	if (image_getWidth(target) != image_getWidth(source) && image_getHeight(target) > image_getHeight(source)) {
		// Upscaling is faster in two steps by both reusing the horizontal interpolation and vectorizing the vertical interpolation.
		int tempWidth = image_getWidth(target);
		int tempHeight = image_getHeight(source);
		IRect tempScaleRegion = IRect(scaleRegion.left(), 0, scaleRegion.width(), image_getHeight(source));
		// Create a temporary buffer.
		IMAGE_TYPE newTempImage = createWithSamePackOrder(target, tempWidth, tempHeight);
		resize_aux(newTempImage, source, interpolate, tempScaleRegion);
		resize_aux(target, newTempImage, interpolate, scaleRegion);
	} else {
		// Downscaling or only changing one dimension is faster in one step.
		resize_aux(target, source, interpolate, scaleRegion);
	}
}

template <bool CONVERT_COLOR>
static inline uint32_t convertRead(const ImageRgbaU8& target, const ImageRgbaU8& source, int x, int y) {
	uint32_t result = image_readPixel_clamp_packed(source, x, y);
	if (CONVERT_COLOR) {
		result = image_truncateAndPack(target, image_unpack(source, result));
	}
	return result;
}

// Used for drawing large pixels
static inline void fillRectangle(const ImageRgbaU8& target, int pixelLeft, int pixelRight, int pixelTop, int pixelBottom, const uint32_t& packedColor) {
	SafePointer<uint32_t> targetRow = image_getSafePointer<uint32_t>(target, pixelTop) + pixelLeft;
	for (int y = pixelTop; y < pixelBottom; y++) {
		SafePointer<uint32_t> targetPixel = targetRow;
		for (int x = pixelLeft; x < pixelRight; x++) {
			*targetPixel = packedColor;
			targetPixel += 1;
		}
		targetRow.increaseBytes(image_getStride(target));
	}
}

template <bool CONVERT_COLOR>
static void blockMagnify_reference(
  const ImageRgbaU8& target, const ImageRgbaU8& source,
  int pixelWidth, int pixelHeight, int clipWidth, int clipHeight) {
	int sourceY = 0;
	int maxSourceX = image_getWidth(source) - 1;
	int maxSourceY = image_getHeight(source) - 1;
	if (clipWidth > image_getWidth(target)) { clipWidth = image_getWidth(target); }
	if (clipHeight > image_getHeight(target)) { clipHeight = image_getHeight(target); }
	for (int32_t pixelTop = 0; pixelTop < clipHeight; pixelTop += pixelHeight) {
		int sourceX = 0;
		for (int32_t pixelLeft = 0; pixelLeft < clipWidth; pixelLeft += pixelWidth) {
			// Read the pixel once
			uint32_t sourceColor = convertRead<CONVERT_COLOR>(target, source, sourceX, sourceY);
			// Write to all target pixels in a conditionless loop
			fillRectangle(target, pixelLeft, pixelLeft + pixelWidth, pixelTop, pixelTop + pixelHeight, sourceColor);
			// Iterate and clamp the read coordinate
			sourceX++;
			if (sourceX > maxSourceX) { sourceX = maxSourceX; }
		}
		// Iterate and clamp the read coordinate
		sourceY++;
		if (sourceY > maxSourceY) { sourceY = maxSourceY; }
	}
}

// Pre-condition:
//   * The source and target images have the same pack order
//   * Both source and target are 16-byte aligned, but does not have to own their padding
//   * clipWidth % 2 == 0
//   * clipHeight % 2 == 0
static void blockMagnify_2x2(const ImageRgbaU8& target, const ImageRgbaU8& source, int clipWidth, int clipHeight) {
	SafePointer<const uint32_t> sourceRow = image_getSafePointer<uint32_t>(source);
	SafePointer<uint32_t> targetRowA = image_getSafePointer<uint32_t>(target, 0);
	SafePointer<uint32_t> targetRowB = image_getSafePointer<uint32_t>(target, 1);
	int blockTargetStride = image_getStride(target) * 2;
	for (int upperTargetY = 0; upperTargetY + 2 <= clipHeight; upperTargetY+=2) {
		// Carriage return
		SafePointer<const uint32_t> sourcePixel = sourceRow;
		SafePointer<uint32_t> targetPixelA = targetRowA;
		SafePointer<uint32_t> targetPixelB = targetRowB;
		// Write to whole multiples of 8 pixels
		int writeLeftX = 0;
		while (writeLeftX + 2 <= clipWidth) {
			// Read one pixel at a time
			uint32_t scalarValue = *sourcePixel;
			sourcePixel += 1;
			// Write to a whole block of pixels
			targetPixelA[0] = scalarValue; targetPixelA[1] = scalarValue;
			targetPixelB[0] = scalarValue; targetPixelB[1] = scalarValue;
			targetPixelA += 2;
			targetPixelB += 2;
			// Count
			writeLeftX += 2;
		}
		// Line feed
		sourceRow.increaseBytes(image_getStride(source));
		targetRowA.increaseBytes(blockTargetStride);
		targetRowB.increaseBytes(blockTargetStride);
	}
}

// Pre-condition:
//   * The source and target images have the same pack order
//   * Both source and target are 16-byte aligned, but does not have to own their padding
//   * clipWidth % 3 == 0
//   * clipHeight % 3 == 0
static void blockMagnify_3x3(const ImageRgbaU8& target, const ImageRgbaU8& source, int clipWidth, int clipHeight) {
	SafePointer<const uint32_t> sourceRow = image_getSafePointer<uint32_t>(source);
	SafePointer<uint32_t> targetRowA = image_getSafePointer<uint32_t>(target, 0);
	SafePointer<uint32_t> targetRowB = image_getSafePointer<uint32_t>(target, 1);
	SafePointer<uint32_t> targetRowC = image_getSafePointer<uint32_t>(target, 2);
	int blockTargetStride = image_getStride(target) * 3;
	for (int upperTargetY = 0; upperTargetY + 3 <= clipHeight; upperTargetY+=3) {
		// Carriage return
		SafePointer<const uint32_t> sourcePixel = sourceRow;
		SafePointer<uint32_t> targetPixelA = targetRowA;
		SafePointer<uint32_t> targetPixelB = targetRowB;
		SafePointer<uint32_t> targetPixelC = targetRowC;
		int writeLeftX = 0;
		while (writeLeftX + 3 <= clipWidth) {
			// Read one pixel at a time
			uint32_t scalarValue = *sourcePixel;
			sourcePixel += 1;
			// Write to a whole block of pixels
			targetPixelA[0] = scalarValue; targetPixelA[1] = scalarValue; targetPixelA[2] = scalarValue;
			targetPixelB[0] = scalarValue; targetPixelB[1] = scalarValue; targetPixelB[2] = scalarValue;
			targetPixelC[0] = scalarValue; targetPixelC[1] = scalarValue; targetPixelC[2] = scalarValue;
			targetPixelA += 3;
			targetPixelB += 3;
			targetPixelC += 3;
			// Count
			writeLeftX += 3;
		}
		// Line feed
		sourceRow.increaseBytes(image_getStride(source));
		targetRowA.increaseBytes(blockTargetStride);
		targetRowB.increaseBytes(blockTargetStride);
		targetRowC.increaseBytes(blockTargetStride);
	}
}

// Pre-condition:
//   * The source and target images have the same pack order
//   * Both source and target are 16-byte aligned, but does not have to own their padding
//   * clipWidth % 4 == 0
//   * clipHeight % 4 == 0
static void blockMagnify_4x4(const ImageRgbaU8& target, const ImageRgbaU8& source, int clipWidth, int clipHeight) {
	SafePointer<const uint32_t> sourceRow = image_getSafePointer<uint32_t>(source);
	SafePointer<uint32_t> targetRowA = image_getSafePointer<uint32_t>(target, 0);
	SafePointer<uint32_t> targetRowB = image_getSafePointer<uint32_t>(target, 1);
	SafePointer<uint32_t> targetRowC = image_getSafePointer<uint32_t>(target, 2);
	SafePointer<uint32_t> targetRowD = image_getSafePointer<uint32_t>(target, 3);
	int quadTargetStride = image_getStride(target) * 4;
	for (int upperTargetY = 0; upperTargetY + 4 <= clipHeight; upperTargetY+=4) {
		// Carriage return
		SafePointer<const uint32_t> sourcePixel = sourceRow;
		SafePointer<uint32_t> targetPixelA = targetRowA;
		SafePointer<uint32_t> targetPixelB = targetRowB;
		SafePointer<uint32_t> targetPixelC = targetRowC;
		SafePointer<uint32_t> targetPixelD = targetRowD;
		int writeLeftX = 0;
		while (writeLeftX + 4 <= clipWidth) {
			// Read one pixel at a time
			uint32_t scalarValue = *sourcePixel;
			sourcePixel += 1;
			// Convert scalar to SIMD vector of 4 repeated pixels
			ALIGN16 U32x4 sourcePixels = U32x4(scalarValue);
			// Write to 4x4 pixels using 4 SIMD writes
			sourcePixels.writeAligned(targetPixelA, "blockMagnify_4x4 @ write A");
			sourcePixels.writeAligned(targetPixelB, "blockMagnify_4x4 @ write B");
			sourcePixels.writeAligned(targetPixelC, "blockMagnify_4x4 @ write C");
			sourcePixels.writeAligned(targetPixelD, "blockMagnify_4x4 @ write D");
			targetPixelA += 4;
			targetPixelB += 4;
			targetPixelC += 4;
			targetPixelD += 4;
			// Count
			writeLeftX += 4;
		}
		// Line feed
		sourceRow.increaseBytes(image_getStride(source));
		targetRowA.increaseBytes(quadTargetStride);
		targetRowB.increaseBytes(quadTargetStride);
		targetRowC.increaseBytes(quadTargetStride);
		targetRowD.increaseBytes(quadTargetStride);
	}
}

// Pre-condition:
//   * The source and target images have the same pack order
//   * Both source and target are 16-byte aligned, but does not have to own their padding
//   * clipWidth % 5 == 0
//   * clipHeight % 5 == 0
static void blockMagnify_5x5(const ImageRgbaU8& target, const ImageRgbaU8& source, int clipWidth, int clipHeight) {
	SafePointer<const uint32_t> sourceRow = image_getSafePointer<uint32_t>(source);
	SafePointer<uint32_t> targetRowA = image_getSafePointer<uint32_t>(target, 0);
	SafePointer<uint32_t> targetRowB = image_getSafePointer<uint32_t>(target, 1);
	SafePointer<uint32_t> targetRowC = image_getSafePointer<uint32_t>(target, 2);
	SafePointer<uint32_t> targetRowD = image_getSafePointer<uint32_t>(target, 3);
	SafePointer<uint32_t> targetRowE = image_getSafePointer<uint32_t>(target, 4);
	int blockTargetStride = image_getStride(target) * 5;
	for (int upperTargetY = 0; upperTargetY + 5 <= clipHeight; upperTargetY+=5) {
		// Carriage return
		SafePointer<const uint32_t> sourcePixel = sourceRow;
		SafePointer<uint32_t> targetPixelA = targetRowA;
		SafePointer<uint32_t> targetPixelB = targetRowB;
		SafePointer<uint32_t> targetPixelC = targetRowC;
		SafePointer<uint32_t> targetPixelD = targetRowD;
		SafePointer<uint32_t> targetPixelE = targetRowE;
		int writeLeftX = 0;
		while (writeLeftX + 5 <= clipWidth) {
			// Read one pixel at a time
			uint32_t scalarValue = *sourcePixel;
			sourcePixel += 1;
			// Write to a whole block of pixels
			targetPixelA[0] = scalarValue; targetPixelA[1] = scalarValue; targetPixelA[2] = scalarValue; targetPixelA[3] = scalarValue; targetPixelA[4] = scalarValue;
			targetPixelB[0] = scalarValue; targetPixelB[1] = scalarValue; targetPixelB[2] = scalarValue; targetPixelB[3] = scalarValue; targetPixelB[4] = scalarValue;
			targetPixelC[0] = scalarValue; targetPixelC[1] = scalarValue; targetPixelC[2] = scalarValue; targetPixelC[3] = scalarValue; targetPixelC[4] = scalarValue;
			targetPixelD[0] = scalarValue; targetPixelD[1] = scalarValue; targetPixelD[2] = scalarValue; targetPixelD[3] = scalarValue; targetPixelD[4] = scalarValue;
			targetPixelE[0] = scalarValue; targetPixelE[1] = scalarValue; targetPixelE[2] = scalarValue; targetPixelE[3] = scalarValue; targetPixelE[4] = scalarValue;
			targetPixelA += 5;
			targetPixelB += 5;
			targetPixelC += 5;
			targetPixelD += 5;
			targetPixelE += 5;
			// Count
			writeLeftX += 5;
		}
		// Line feed
		sourceRow.increaseBytes(image_getStride(source));
		targetRowA.increaseBytes(blockTargetStride);
		targetRowB.increaseBytes(blockTargetStride);
		targetRowC.increaseBytes(blockTargetStride);
		targetRowD.increaseBytes(blockTargetStride);
		targetRowE.increaseBytes(blockTargetStride);
	}
}

// Pre-condition:
//   * The source and target images have the same pack order
//   * Both source and target are 16-byte aligned, but does not have to own their padding
//   * clipWidth % 6 == 0
//   * clipHeight % 6 == 0
static void blockMagnify_6x6(const ImageRgbaU8& target, const ImageRgbaU8& source, int clipWidth, int clipHeight) {
	SafePointer<const uint32_t> sourceRow = image_getSafePointer<uint32_t>(source);
	SafePointer<uint32_t> targetRowA = image_getSafePointer<uint32_t>(target, 0);
	SafePointer<uint32_t> targetRowB = image_getSafePointer<uint32_t>(target, 1);
	SafePointer<uint32_t> targetRowC = image_getSafePointer<uint32_t>(target, 2);
	SafePointer<uint32_t> targetRowD = image_getSafePointer<uint32_t>(target, 3);
	SafePointer<uint32_t> targetRowE = image_getSafePointer<uint32_t>(target, 4);
	SafePointer<uint32_t> targetRowF = image_getSafePointer<uint32_t>(target, 5);
	int blockTargetStride = image_getStride(target) * 6;
	for (int upperTargetY = 0; upperTargetY + 6 <= clipHeight; upperTargetY+=6) {
		// Carriage return
		SafePointer<const uint32_t> sourcePixel = sourceRow;
		SafePointer<uint32_t> targetPixelA = targetRowA;
		SafePointer<uint32_t> targetPixelB = targetRowB;
		SafePointer<uint32_t> targetPixelC = targetRowC;
		SafePointer<uint32_t> targetPixelD = targetRowD;
		SafePointer<uint32_t> targetPixelE = targetRowE;
		SafePointer<uint32_t> targetPixelF = targetRowF;
		int writeLeftX = 0;
		while (writeLeftX + 6 <= clipWidth) {
			// Read one pixel at a time
			uint32_t scalarValue = *sourcePixel;
			sourcePixel += 1;
			// Write to a whole block of pixels
			targetPixelA[0] = scalarValue; targetPixelA[1] = scalarValue; targetPixelA[2] = scalarValue; targetPixelA[3] = scalarValue; targetPixelA[4] = scalarValue; targetPixelA[5] = scalarValue;
			targetPixelB[0] = scalarValue; targetPixelB[1] = scalarValue; targetPixelB[2] = scalarValue; targetPixelB[3] = scalarValue; targetPixelB[4] = scalarValue; targetPixelB[5] = scalarValue;
			targetPixelC[0] = scalarValue; targetPixelC[1] = scalarValue; targetPixelC[2] = scalarValue; targetPixelC[3] = scalarValue; targetPixelC[4] = scalarValue; targetPixelC[5] = scalarValue;
			targetPixelD[0] = scalarValue; targetPixelD[1] = scalarValue; targetPixelD[2] = scalarValue; targetPixelD[3] = scalarValue; targetPixelD[4] = scalarValue; targetPixelD[5] = scalarValue;
			targetPixelE[0] = scalarValue; targetPixelE[1] = scalarValue; targetPixelE[2] = scalarValue; targetPixelE[3] = scalarValue; targetPixelE[4] = scalarValue; targetPixelE[5] = scalarValue;
			targetPixelF[0] = scalarValue; targetPixelF[1] = scalarValue; targetPixelF[2] = scalarValue; targetPixelF[3] = scalarValue; targetPixelF[4] = scalarValue; targetPixelF[5] = scalarValue;
			targetPixelA += 6;
			targetPixelB += 6;
			targetPixelC += 6;
			targetPixelD += 6;
			targetPixelE += 6;
			targetPixelF += 6;
			// Count
			writeLeftX += 6;
		}
		// Line feed
		sourceRow.increaseBytes(image_getStride(source));
		targetRowA.increaseBytes(blockTargetStride);
		targetRowB.increaseBytes(blockTargetStride);
		targetRowC.increaseBytes(blockTargetStride);
		targetRowD.increaseBytes(blockTargetStride);
		targetRowE.increaseBytes(blockTargetStride);
		targetRowF.increaseBytes(blockTargetStride);
	}
}

// Pre-condition:
//   * The source and target images have the same pack order
//   * Both source and target are 16-byte aligned, but does not have to own their padding
//   * clipWidth % 7 == 0
//   * clipHeight % 7 == 0
static void blockMagnify_7x7(const ImageRgbaU8& target, const ImageRgbaU8& source, int clipWidth, int clipHeight) {
	SafePointer<const uint32_t> sourceRow = image_getSafePointer<uint32_t>(source);
	SafePointer<uint32_t> targetRowA = image_getSafePointer<uint32_t>(target, 0);
	SafePointer<uint32_t> targetRowB = image_getSafePointer<uint32_t>(target, 1);
	SafePointer<uint32_t> targetRowC = image_getSafePointer<uint32_t>(target, 2);
	SafePointer<uint32_t> targetRowD = image_getSafePointer<uint32_t>(target, 3);
	SafePointer<uint32_t> targetRowE = image_getSafePointer<uint32_t>(target, 4);
	SafePointer<uint32_t> targetRowF = image_getSafePointer<uint32_t>(target, 5);
	SafePointer<uint32_t> targetRowG = image_getSafePointer<uint32_t>(target, 6);
	int blockTargetStride = image_getStride(target) * 7;
	for (int upperTargetY = 0; upperTargetY + 7 <= clipHeight; upperTargetY+=7) {
		// Carriage return
		SafePointer<const uint32_t> sourcePixel = sourceRow;
		SafePointer<uint32_t> targetPixelA = targetRowA;
		SafePointer<uint32_t> targetPixelB = targetRowB;
		SafePointer<uint32_t> targetPixelC = targetRowC;
		SafePointer<uint32_t> targetPixelD = targetRowD;
		SafePointer<uint32_t> targetPixelE = targetRowE;
		SafePointer<uint32_t> targetPixelF = targetRowF;
		SafePointer<uint32_t> targetPixelG = targetRowG;
		int writeLeftX = 0;
		while (writeLeftX + 7 <= clipWidth) {
			// Read one pixel at a time
			uint32_t scalarValue = *sourcePixel;
			sourcePixel += 1;
			// Write to a whole block of pixels
			targetPixelA[0] = scalarValue; targetPixelA[1] = scalarValue; targetPixelA[2] = scalarValue; targetPixelA[3] = scalarValue; targetPixelA[4] = scalarValue; targetPixelA[5] = scalarValue; targetPixelA[6] = scalarValue;
			targetPixelB[0] = scalarValue; targetPixelB[1] = scalarValue; targetPixelB[2] = scalarValue; targetPixelB[3] = scalarValue; targetPixelB[4] = scalarValue; targetPixelB[5] = scalarValue; targetPixelB[6] = scalarValue;
			targetPixelC[0] = scalarValue; targetPixelC[1] = scalarValue; targetPixelC[2] = scalarValue; targetPixelC[3] = scalarValue; targetPixelC[4] = scalarValue; targetPixelC[5] = scalarValue; targetPixelC[6] = scalarValue;
			targetPixelD[0] = scalarValue; targetPixelD[1] = scalarValue; targetPixelD[2] = scalarValue; targetPixelD[3] = scalarValue; targetPixelD[4] = scalarValue; targetPixelD[5] = scalarValue; targetPixelD[6] = scalarValue;
			targetPixelE[0] = scalarValue; targetPixelE[1] = scalarValue; targetPixelE[2] = scalarValue; targetPixelE[3] = scalarValue; targetPixelE[4] = scalarValue; targetPixelE[5] = scalarValue; targetPixelE[6] = scalarValue;
			targetPixelF[0] = scalarValue; targetPixelF[1] = scalarValue; targetPixelF[2] = scalarValue; targetPixelF[3] = scalarValue; targetPixelF[4] = scalarValue; targetPixelF[5] = scalarValue; targetPixelF[6] = scalarValue;
			targetPixelG[0] = scalarValue; targetPixelG[1] = scalarValue; targetPixelG[2] = scalarValue; targetPixelG[3] = scalarValue; targetPixelG[4] = scalarValue; targetPixelG[5] = scalarValue; targetPixelG[6] = scalarValue;
			targetPixelA += 7;
			targetPixelB += 7;
			targetPixelC += 7;
			targetPixelD += 7;
			targetPixelE += 7;
			targetPixelF += 7;
			targetPixelG += 7;
			// Count
			writeLeftX += 7;
		}
		// Line feed
		sourceRow.increaseBytes(image_getStride(source));
		targetRowA.increaseBytes(blockTargetStride);
		targetRowB.increaseBytes(blockTargetStride);
		targetRowC.increaseBytes(blockTargetStride);
		targetRowD.increaseBytes(blockTargetStride);
		targetRowE.increaseBytes(blockTargetStride);
		targetRowF.increaseBytes(blockTargetStride);
		targetRowG.increaseBytes(blockTargetStride);
	}
}

// Pre-condition:
//   * The source and target images have the same pack order
//   * Both source and target are 16-byte aligned, but does not have to own their padding
//   * clipWidth % 8 == 0
//   * clipHeight % 8 == 0
static void blockMagnify_8x8(const ImageRgbaU8& target, const ImageRgbaU8& source, int clipWidth, int clipHeight) {
	SafePointer<const uint32_t> sourceRow = image_getSafePointer<uint32_t>(source);
	SafePointer<uint32_t> targetRowA = image_getSafePointer<uint32_t>(target, 0);
	SafePointer<uint32_t> targetRowB = image_getSafePointer<uint32_t>(target, 1);
	SafePointer<uint32_t> targetRowC = image_getSafePointer<uint32_t>(target, 2);
	SafePointer<uint32_t> targetRowD = image_getSafePointer<uint32_t>(target, 3);
	SafePointer<uint32_t> targetRowE = image_getSafePointer<uint32_t>(target, 4);
	SafePointer<uint32_t> targetRowF = image_getSafePointer<uint32_t>(target, 5);
	SafePointer<uint32_t> targetRowG = image_getSafePointer<uint32_t>(target, 6);
	SafePointer<uint32_t> targetRowH = image_getSafePointer<uint32_t>(target, 7);
	int blockTargetStride = image_getStride(target) * 8;
	for (int upperTargetY = 0; upperTargetY + 8 <= clipHeight; upperTargetY+=8) {
		// Carriage return
		SafePointer<const uint32_t> sourcePixel = sourceRow;
		SafePointer<uint32_t> targetPixelA = targetRowA;
		SafePointer<uint32_t> targetPixelB = targetRowB;
		SafePointer<uint32_t> targetPixelC = targetRowC;
		SafePointer<uint32_t> targetPixelD = targetRowD;
		SafePointer<uint32_t> targetPixelE = targetRowE;
		SafePointer<uint32_t> targetPixelF = targetRowF;
		SafePointer<uint32_t> targetPixelG = targetRowG;
		SafePointer<uint32_t> targetPixelH = targetRowH;
		int writeLeftX = 0;
		while (writeLeftX + 8 <= clipWidth) {
			// Read one pixel at a time
			uint32_t scalarValue = *sourcePixel;
			sourcePixel += 1;
			// Write to a whole block of pixels
			targetPixelA[0] = scalarValue; targetPixelA[1] = scalarValue; targetPixelA[2] = scalarValue; targetPixelA[3] = scalarValue; targetPixelA[4] = scalarValue; targetPixelA[5] = scalarValue; targetPixelA[6] = scalarValue; targetPixelA[7] = scalarValue;
			targetPixelB[0] = scalarValue; targetPixelB[1] = scalarValue; targetPixelB[2] = scalarValue; targetPixelB[3] = scalarValue; targetPixelB[4] = scalarValue; targetPixelB[5] = scalarValue; targetPixelB[6] = scalarValue; targetPixelB[7] = scalarValue;
			targetPixelC[0] = scalarValue; targetPixelC[1] = scalarValue; targetPixelC[2] = scalarValue; targetPixelC[3] = scalarValue; targetPixelC[4] = scalarValue; targetPixelC[5] = scalarValue; targetPixelC[6] = scalarValue; targetPixelC[7] = scalarValue;
			targetPixelD[0] = scalarValue; targetPixelD[1] = scalarValue; targetPixelD[2] = scalarValue; targetPixelD[3] = scalarValue; targetPixelD[4] = scalarValue; targetPixelD[5] = scalarValue; targetPixelD[6] = scalarValue; targetPixelD[7] = scalarValue;
			targetPixelE[0] = scalarValue; targetPixelE[1] = scalarValue; targetPixelE[2] = scalarValue; targetPixelE[3] = scalarValue; targetPixelE[4] = scalarValue; targetPixelE[5] = scalarValue; targetPixelE[6] = scalarValue; targetPixelE[7] = scalarValue;
			targetPixelF[0] = scalarValue; targetPixelF[1] = scalarValue; targetPixelF[2] = scalarValue; targetPixelF[3] = scalarValue; targetPixelF[4] = scalarValue; targetPixelF[5] = scalarValue; targetPixelF[6] = scalarValue; targetPixelF[7] = scalarValue;
			targetPixelG[0] = scalarValue; targetPixelG[1] = scalarValue; targetPixelG[2] = scalarValue; targetPixelG[3] = scalarValue; targetPixelG[4] = scalarValue; targetPixelG[5] = scalarValue; targetPixelG[6] = scalarValue; targetPixelG[7] = scalarValue;
			targetPixelH[0] = scalarValue; targetPixelH[1] = scalarValue; targetPixelH[2] = scalarValue; targetPixelH[3] = scalarValue; targetPixelH[4] = scalarValue; targetPixelH[5] = scalarValue; targetPixelH[6] = scalarValue; targetPixelH[7] = scalarValue;
			targetPixelA += 8;
			targetPixelB += 8;
			targetPixelC += 8;
			targetPixelD += 8;
			targetPixelE += 8;
			targetPixelF += 8;
			targetPixelG += 8;
			targetPixelH += 8;
			// Count
			writeLeftX += 8;
		}
		// Line feed
		sourceRow.increaseBytes(image_getStride(source));
		targetRowA.increaseBytes(blockTargetStride);
		targetRowB.increaseBytes(blockTargetStride);
		targetRowC.increaseBytes(blockTargetStride);
		targetRowD.increaseBytes(blockTargetStride);
		targetRowE.increaseBytes(blockTargetStride);
		targetRowF.increaseBytes(blockTargetStride);
		targetRowG.increaseBytes(blockTargetStride);
		targetRowH.increaseBytes(blockTargetStride);
	}
}

static void blackEdges(const ImageRgbaU8& target, int excludedWidth, int excludedHeight) {
	// Right side
	draw_rectangle(target, IRect(excludedWidth, 0, image_getWidth(target) - excludedWidth, excludedHeight), 0);
	// Bottom and corner
	draw_rectangle(target, IRect(0, excludedHeight, image_getWidth(target), image_getHeight(target) - excludedHeight), 0);
}

static void imageImpl_blockMagnify(const ImageRgbaU8& target, const ImageRgbaU8& source, int pixelWidth, int pixelHeight) {
	if (pixelWidth < 1) { pixelWidth = 1; }
	if (pixelHeight < 1) { pixelHeight = 1; }
	bool sameOrder = image_getPackOrderIndex(target) == image_getPackOrderIndex(source);
	// Find the part of source which fits into target with whole pixels
	int clipWidth = roundDown(min(image_getWidth(target), image_getWidth(source) * pixelWidth), pixelWidth);
	int clipHeight = roundDown(min(image_getHeight(target), image_getHeight(source) * pixelHeight), pixelHeight);
	if (sameOrder) {
		if (!(image_isSubImage(source) || image_isSubImage(target))) {
			if (pixelWidth == 2 && pixelHeight == 2) {
				blockMagnify_2x2(target, source, clipWidth, clipHeight);
			} else if (pixelWidth == 3 && pixelHeight == 3) {
				blockMagnify_3x3(target, source, clipWidth, clipHeight);
			} else if (pixelWidth == 4 && pixelHeight == 4) {
				blockMagnify_4x4(target, source, clipWidth, clipHeight);
			} else if (pixelWidth == 5 && pixelHeight == 5) {
				blockMagnify_5x5(target, source, clipWidth, clipHeight);
			} else if (pixelWidth == 6 && pixelHeight == 6) {
				blockMagnify_6x6(target, source, clipWidth, clipHeight);
			} else if (pixelWidth == 7 && pixelHeight == 7) {
				blockMagnify_7x7(target, source, clipWidth, clipHeight);
			} else if (pixelWidth == 8 && pixelHeight == 8) {
				blockMagnify_8x8(target, source, clipWidth, clipHeight);
			} else {
				blockMagnify_reference<false>(target, source, pixelWidth, pixelHeight, clipWidth, clipHeight);
			}
		} else {
			blockMagnify_reference<false>(target, source, pixelWidth, pixelHeight, clipWidth, clipHeight);
		}
	} else {
		blockMagnify_reference<true>(target, source, pixelWidth, pixelHeight, clipWidth, clipHeight);
	}
	blackEdges(target, clipWidth, clipHeight);
}

static void mapRgbaU8(const ImageRgbaU8& target, const ImageGenRgbaU8& lambda, int startX, int startY) {
	const int targetWidth = image_getWidth(target);
	const int targetHeight = image_getHeight(target);
	const int targetStride = image_getStride(target);
	SafePointer<uint32_t> targetRow = image_getSafePointer<uint32_t>(target);
	for (int y = startY; y < targetHeight + startY; y++) {
		SafePointer<uint32_t> targetPixel = targetRow;
		for (int x = startX; x < targetWidth + startX; x++) {
			*targetPixel = image_saturateAndPack(target, lambda(x, y));
			targetPixel += 1;
		}
		targetRow.increaseBytes(targetStride);
	}
}
void filter_mapRgbaU8(const ImageRgbaU8 target, const ImageGenRgbaU8& lambda, int startX, int startY) {
	if (image_exists(target)) {
		mapRgbaU8(target, lambda, startX, startY);
	}
}
OrderedImageRgbaU8 filter_generateRgbaU8(int width, int height, const ImageGenRgbaU8& lambda, int startX, int startY) {
	OrderedImageRgbaU8 result = image_create_RgbaU8(width, height);
	filter_mapRgbaU8(result, lambda, startX, startY);
	return result;
}

template <typename IMAGE_TYPE, typename PIXEL_TYPE, int MIN_VALUE, int MAX_VALUE>
static void mapMonochrome(const IMAGE_TYPE& target, const ImageGenI32& lambda, int startX, int startY) {
	const int targetWidth = image_getWidth(target);
	const int targetHeight = image_getHeight(target);
	const int targetStride = image_getStride(target);
	SafePointer<PIXEL_TYPE> targetRow = image_getSafePointer<PIXEL_TYPE>(target);
	for (int y = startY; y < targetHeight + startY; y++) {
		SafePointer<PIXEL_TYPE> targetPixel = targetRow;
		for (int x = startX; x < targetWidth + startX; x++) {
			int output = lambda(x, y);
			if (output < MIN_VALUE) { output = MIN_VALUE; }
			if (output > MAX_VALUE) { output = MAX_VALUE; }
			*targetPixel = output;
			targetPixel += 1;
		}
		targetRow.increaseBytes(targetStride);
	}
}
void filter_mapU8(const ImageU8 target, const ImageGenI32& lambda, int startX, int startY) {
	if (image_exists(target)) {
		mapMonochrome<ImageU8, uint8_t, 0, 255>(target, lambda, startX, startY);
	}
}
AlignedImageU8 filter_generateU8(int width, int height, const ImageGenI32& lambda, int startX, int startY) {
	AlignedImageU8 result = image_create_U8(width, height);
	filter_mapU8(result, lambda, startX, startY);
	return result;
}
void filter_mapU16(const ImageU16 target, const ImageGenI32& lambda, int startX, int startY) {
	if (image_exists(target)) {
		mapMonochrome<ImageU16, uint16_t, 0, 65535>(target, lambda, startX, startY);
	}
}
AlignedImageU16 filter_generateU16(int width, int height, const ImageGenI32& lambda, int startX, int startY) {
	AlignedImageU16 result = image_create_U16(width, height);
	filter_mapU16(result, lambda, startX, startY);
	return result;
}

static void mapF32(const ImageF32& target, const ImageGenF32& lambda, int startX, int startY) {
	const int targetWidth = image_getWidth(target);
	const int targetHeight = image_getHeight(target);
	const int targetStride = image_getStride(target);
	SafePointer<float> targetRow = image_getSafePointer<float>(target);
	for (int y = startY; y < targetHeight + startY; y++) {
		SafePointer<float> targetPixel = targetRow;
		for (int x = startX; x < targetWidth + startX; x++) {
			*targetPixel = lambda(x, y);
			targetPixel += 1;
		}
		targetRow.increaseBytes(targetStride);
	}
}
void filter_mapF32(const ImageF32 target, const ImageGenF32& lambda, int startX, int startY) {
	if (image_exists(target)) {
		mapF32(target, lambda, startX, startY);
	}
}
AlignedImageF32 filter_generateF32(int width, int height, const ImageGenF32& lambda, int startX, int startY) {
	AlignedImageF32 result = image_create_F32(width, height);
	filter_mapF32(result, lambda, startX, startY);
	return result;
}


// -------------------------------- Resize --------------------------------


OrderedImageRgbaU8 filter_resize(const ImageRgbaU8 &source, Sampler interpolation, int32_t newWidth, int32_t newHeight) {
	if (image_exists(source)) {
		OrderedImageRgbaU8 resultImage = image_create_RgbaU8(newWidth, newHeight);
		resizeToTarget<ImageRgbaU8>(resultImage, source, interpolation == Sampler::Linear);
		return resultImage;
	} else {
		return OrderedImageRgbaU8(); // Null gives null
	}
}

AlignedImageU8 filter_resize(const ImageU8 &source, Sampler interpolation, int32_t newWidth, int32_t newHeight) {
	if (image_exists(source)) {
		AlignedImageU8 resultImage = image_create_U8(newWidth, newHeight);
		resizeToTarget<ImageU8>(resultImage, source, interpolation == Sampler::Linear);
		return resultImage;
	} else {
		return AlignedImageU8(); // Null gives null
	}
}

void filter_blockMagnify(const ImageRgbaU8 &target, const ImageRgbaU8& source, int pixelWidth, int pixelHeight) {
	if (image_exists(target) && image_exists(source)) {
		imageImpl_blockMagnify(target, source, pixelWidth, pixelHeight);
	}
}

}
