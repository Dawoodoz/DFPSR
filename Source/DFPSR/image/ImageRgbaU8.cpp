// zlib open source license
//
// Copyright (c) 2017 to 2023 David Forsgren Piuva
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

#include "ImageRgbaU8.h"
#include "internal/imageInternal.h"
#include "internal/imageTemplate.h"
#include "draw.h"
#include <algorithm>
#include "../base/simd.h"

using namespace dsr;

static const int pixelSize = 4;

IMAGE_DEFINITION(ImageRgbaU8Impl, pixelSize, Color4xU8, uint8_t);

ImageRgbaU8Impl::ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight, int32_t newStride, Buffer buffer, intptr_t startOffset, const PackOrder &packOrder) :
  ImageImpl(newWidth, newHeight, newStride, sizeof(Color4xU8), buffer, startOffset), packOrder(packOrder) {
	assert(buffer_getSize(buffer) - startOffset >= imageInternal::getUsedBytes(this));
	this->initializeRgbaImage();
}

ImageRgbaU8Impl::ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight) :
  ImageImpl(newWidth, newHeight, roundUp(newWidth * sizeof(Color4xU8), DSR_MAXIMUM_ALIGNMENT), sizeof(Color4xU8)) {
	this->initializeRgbaImage();
}

// Native canvas constructor
ImageRgbaU8Impl::ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight, PackOrderIndex packOrderIndex) :
  ImageImpl(newWidth, newHeight, roundUp(newWidth * sizeof(Color4xU8), DSR_MAXIMUM_ALIGNMENT), sizeof(Color4xU8)) {
	this->packOrder = PackOrder::getPackOrder(packOrderIndex);
	this->initializeRgbaImage();
}

bool ImageRgbaU8Impl::isTexture() const {
	return this->texture.exists();
}

bool ImageRgbaU8Impl::isTexture(const ImageRgbaU8Impl* image) {
	return image ? image->texture.exists() : false;
}

ImageRgbaU8Impl ImageRgbaU8Impl::getWithoutPadding() const {
	if (this->stride == this->width * pixelSize) {
		// No padding
		return *this;
	} else {
		// Copy each row without padding
		ImageRgbaU8Impl result = ImageRgbaU8Impl(this->width, this->height, this->packOrder.packOrderIndex);
		const SafePointer<uint8_t> sourceRow = imageInternal::getSafeData<uint8_t>(*this);
		int32_t sourceStride = this->stride;
		SafePointer<uint8_t> targetRow = imageInternal::getSafeData<uint8_t>(result);
		int32_t targetStride = result.stride;
		for (int32_t y = 0; y < this->height; y++) {
			safeMemoryCopy(targetRow, sourceRow, targetStride);
			sourceRow += sourceStride;
			targetRow += targetStride;
		}
		return result;
	}
}

static void extractChannel(SafePointer<uint8_t> targetData, int targetStride, const SafePointer<uint8_t> sourceData, int sourceStride, int sourceChannels, int channelIndex, int width, int height) {
	const SafePointer<uint8_t> sourceRow = sourceData + channelIndex;
	SafePointer<uint8_t> targetRow = targetData;
	for (int y = 0; y < height; y++) {
		const SafePointer<uint8_t> sourceElement = sourceRow;
		SafePointer<uint8_t> targetElement = targetRow;
		for (int x = 0; x < width; x++) {
			*targetElement = *sourceElement; // Copy one channel from the soruce
			sourceElement += sourceChannels; // Jump to the same channel in the next source pixel
			targetElement += 1; // Jump to the next monochrome target pixel
		}
		sourceRow.increaseBytes(sourceStride);
		targetRow.increaseBytes(targetStride);
	}
}

ImageU8Impl ImageRgbaU8Impl::getChannel(int32_t channelIndex) const {
	// Warning for debug mode
	assert(channelIndex >= 0 && channelIndex < channelCount);
	// Safety for release mode
	if (channelIndex < 0) { channelIndex = 0; }
	if (channelIndex > channelCount) { channelIndex = channelCount; }
	ImageU8Impl result(this->width, this->height);
	extractChannel(imageInternal::getSafeData<uint8_t>(result), result.stride, imageInternal::getSafeData<uint8_t>(*this), this->stride, channelCount, channelIndex, this->width, this->height);
	return result;
}

static const int32_t smallestSizeGroup = 5;
static const int32_t largestSizeGroup = 14;
static int32_t getSizeGroup(int32_t size) {
	int32_t group = -1;
	if (size == 1) {
		group = 0; // Too small for 16-byte alignment!
	} else if (size == 2) {
		group = 1; // Too small for 16-byte alignment! (SSE2)
	} else if (size == 4) {
		group = 2; // Too small for 32-byte alignment! (AVX2)
	} else if (size == 8) {
		group = 3; // Too small for 64-byte alignment! (AVX3)
	} else if (size == 16) {
		group = 4; // Too small for 128-byte alignment!
	} else if (size == 32) {
		group = 5; // Smallest allowed texture dimension, allowing 1024-bit SIMD.
	} else if (size == 64) {
		group = 6;
	} else if (size == 128) {
		group = 7;
	} else if (size == 256) {
		group = 8;
	} else if (size == 512) {
		group = 9;
	} else if (size == 1024) {
		group = 10;
	} else if (size == 2048) {
		group = 11;
	} else if (size == 4096) {
		group = 12;
	} else if (size == 8192) {
		group = 13;
	} else if (size == 16384) {
		group = 14; // Largest allowed texture dimension
	} // Higher dimensions should return -1, so that initializeRgbaImage avoids initializing the image as a texture and isTexture returns false
	return group;
}

inline int32_t sizeFromGroup(int32_t group) {
	return 1 << group;
}

// Round the size down, unless it is already too small.
static int32_t roundSize(int32_t size) {
	for (int groupIndex = smallestSizeGroup; groupIndex < largestSizeGroup; groupIndex++) {
		int currentSize = sizeFromGroup(groupIndex);
		if (size < currentSize) {
			return currentSize;
		}
	}
	return sizeFromGroup(largestSizeGroup);
}

static int32_t getPyramidSize(int32_t width, int32_t height, int32_t levels) {
	uint32_t result = 0;
	uint32_t byteCount = width * height * pixelSize;
	for (int32_t l = 0; l < levels; l++) {
		result += byteCount; // Add image size to pyramid size
		byteCount = byteCount >> 2; // Divide size by 4
	}
	return (int32_t)result;
}

inline U32xX averageColor(const U32xX &colorA, const U32xX &colorB) {
	// TODO: Expand to 16 bits or use built in average intrinsics for full bit depth.
	// 7-bit precision for speed.
	return reinterpret_U32FromU8(reinterpret_U8FromU32((colorA >> 1) & U32xX(0b01111111011111110111111101111111)) + reinterpret_U8FromU32((colorB >> 1) & U32xX(0b01111111011111110111111101111111)));
}

inline U32xX pairwiseAverageColor(const U32xX &colorA, const U32xX &colorB) {
	// TODO: Vectorize with 32-bit unzipping of pixels and 8-bit average of channels.
	// Reference implementation
	ALIGN_BYTES(DSR_DEFAULT_ALIGNMENT) uint8_t elementsA[laneCountX_8Bit];
	ALIGN_BYTES(DSR_DEFAULT_ALIGNMENT) uint8_t elementsB[laneCountX_8Bit];
	ALIGN_BYTES(DSR_DEFAULT_ALIGNMENT) uint8_t elementsR[laneCountX_8Bit];
	colorA.writeAlignedUnsafe((uint32_t*)elementsA);
	colorB.writeAlignedUnsafe((uint32_t*)elementsB);
	int32_t halfPixels = laneCountX_32Bit / 2;
	for (int p = 0; p < halfPixels; p++) {
		for (int c = 0; c < 4; c++) {
			elementsR[p * 4 + c] = uint8_t((uint16_t(elementsA[p * 8 + c]) + uint16_t(elementsA[p * 8 + 4 + c])) >> 1);
			elementsR[(p + halfPixels) * 4 + c] = uint8_t((uint16_t(elementsB[p * 8 + c]) + uint16_t(elementsB[p * 8 + 4 + c])) >> 1);
		}
	}
	return U32xX::readAlignedUnsafe((uint32_t*)elementsR);
}

static void downScaleByTwo(SafePointer<uint32_t> targetData, const SafePointer<uint32_t> sourceData, int32_t targetWidth, int32_t targetHeight, int32_t targetStride) {
	int32_t sourceStride = targetStride * 2;
	int32_t doubleSourceStride = sourceStride * 2;
	SafePointer<uint32_t> targetRow = targetData;
	const SafePointer<uint32_t> sourceRow = sourceData;
	for (int32_t y = 0; y < targetHeight; y++) {
		const SafePointer<uint32_t> upperSourcePixel = sourceRow;
		const SafePointer<uint32_t> lowerSourcePixel = sourceRow;
		lowerSourcePixel.increaseBytes(sourceStride);
		SafePointer<uint32_t> targetPixel = targetRow;
		for (int32_t x = 0; x < targetWidth; x += laneCountX_32Bit) {
			U32xX upperLeft = U32xX::readAligned(upperSourcePixel, "upperLeftSource in downScaleByTwo");
			U32xX upperRight = U32xX::readAligned(lowerSourcePixel + laneCountX_32Bit, "upperLeftSource in downScaleByTwo");
			U32xX lowerLeft = U32xX::readAligned(lowerSourcePixel, "upperLeftSource in downScaleByTwo");
			U32xX lowerRight = U32xX::readAligned(lowerSourcePixel + laneCountX_32Bit, "upperLeftSource in downScaleByTwo");
			U32xX upperAverage = pairwiseAverageColor(upperLeft, upperRight);
			U32xX lowerAverage = pairwiseAverageColor(lowerLeft, lowerRight);
			U32xX finalAverage = averageColor(upperAverage, lowerAverage);
			finalAverage.writeAligned(targetPixel, "average result in downScaleByTwo");
			targetPixel += laneCountX_32Bit;
			upperSourcePixel += laneCountX_32Bit * 2;
			lowerSourcePixel += laneCountX_32Bit * 2;
		}
		targetRow.increaseBytes(targetStride);
		sourceRow.increaseBytes(doubleSourceStride);
	}
}

static void updatePyramid(TextureRgba &texture, int32_t layerCount) {
	// Downscale each following layer from the previous.
	for (int32_t targetIndex = 1; targetIndex < layerCount; targetIndex++) {
		int32_t sourceIndex = targetIndex - 1;
		int32_t targetWidth = texture.mips[targetIndex].width;
		int32_t targetHeight = texture.mips[targetIndex].height;
		downScaleByTwo(texture.data + texture.mips[targetIndex].startOffset, texture.data + texture.mips[sourceIndex].startOffset, targetWidth, targetHeight, targetWidth * pixelSize);
	}
	texture.layerCount = layerCount;
}

TextureRgbaLayer::TextureRgbaLayer() {}

TextureRgbaLayer::TextureRgbaLayer(uint32_t startOffset, int32_t width, int32_t height) :
  startOffset(startOffset),
  widthShift(getSizeGroup(width)),
  widthMask(width - 1),
  heightMask(height - 1),
  width(width),
  height(height),
  subWidth(width * 256),
  subHeight(height * 256) {}

void ImageRgbaU8Impl::generatePyramidStructure(int32_t layerCount) {
	int32_t currentWidth = this->width;
	int32_t currentHeight = this->height;
	// Allocate smaller pyramid images within the buffer
	uint32_t currentStart = 0;
	for (int32_t m = 0; m < layerCount; m++) {
		this->texture.mips[m] = TextureRgbaLayer(currentStart, currentWidth, currentHeight);
		currentStart += currentWidth * currentHeight;
		currentWidth /= 2;
		currentHeight /= 2;
	}
	// Fill unused mip levels with duplicates of the last mip level
	for (int32_t m = layerCount; m < MIP_BIN_COUNT; m++) {
		// m - 1 is never negative, because layerCount is clamped to at least 1 and nobody would choose zero for MIP_BIN_COUNT.
		this->texture.mips[m] = this->texture.mips[m - 1];
	}
	this->texture.layerCount = layerCount;
	this->texture.data = imageInternal::getSafeData<uint32_t>(*this);
}

void ImageRgbaU8Impl::removePyramidStructure() {
	// The mip layers have offsets relative to the texture's data pointer, which is already compensating for any offset from any parent image.
	for (int32_t m = 0; m < MIP_BIN_COUNT; m++) {
		this->texture.mips[m] = TextureRgbaLayer(0, this->width, this->height);
	}
	// Declare the old pyramid invalid so that it will not be displayed while rendering, but keep the extra memory for next time it is generated.
	this->texture.layerCount = 1;
	this->texture.data = imageInternal::getSafeData<uint32_t>(*this);
}

void ImageRgbaU8Impl::makeIntoTexture() {
	// Check if the image is a valid texture.
	if (!this->isTexture()) {
		// Get valid dimensions.
		int newWidth = roundSize(this->width);
		int newHeight = roundSize(this->height);
		// Create a new image with the correct dimensions.
		ImageRgbaU8Impl result = ImageRgbaU8Impl(newWidth, newHeight);
		// Resize the image content with bi-linear interpolation.
		imageImpl_resizeToTarget(result, *this, true);
		// Take over the new image's content.
		this->buffer = result.buffer;
		this->width = result.width;
		this->height = result.height;
		this->stride = result.stride;
		this->startOffset = 0; // Starts from the beginning.
		this->isSubImage = false; // No longer sharing buffer with any parent image.
	}
}

void ImageRgbaU8Impl::generatePyramid() {
	int32_t fullSizeGroup = getSizeGroup(std::min(this->width, this->height));
	int32_t layerCount = std::min(std::max(fullSizeGroup - smallestSizeGroup, 1), MIP_BIN_COUNT);
	if (this->texture.layerCount > 1) {
		// Regenerate smaller images without wasting time with any redundant checks,
		//   because the image has already been approved the first time it had the pyramid allocated.
		updatePyramid(this->texture, layerCount);
	} else {
		// In the event of having to correct a bad image into a valid texture, there will be two reallocations.
		this->makeIntoTexture();
		Buffer oldBuffer = this->buffer;
		SafePointer<uint32_t> oldData = buffer_getSafeData<uint32_t>(oldBuffer, "Pyramid generation source") + this->startOffset;
		this->buffer = buffer_create(getPyramidSize(this->width, this->height, layerCount));
		this->generatePyramidStructure(layerCount);
		// Copy the image's old content while assuming that there is no padding.
		safeMemoryCopy(this->texture.data + this->texture.mips[0].startOffset, oldData, this->width * this->height * pixelSize);
		// Generate smaller images.
		updatePyramid(this->texture, layerCount);
		// Once an image had a pyramid generated, the new buffer will remain for as long as the image exists.
		this->texture.layerCount = layerCount;
		// Remove start offset because the old data has been cloned to create the new pyramid image.
		this->startOffset = 0;
	}
}

void ImageRgbaU8Impl::removePyramid() {
	// Duplicate the original image when no longer showing the pyramid.
	this->removePyramidStructure();
}

void ImageRgbaU8Impl::initializeRgbaImage() {
	// If the image fills the criterias of a texture
	if (getSizeGroup(this->width) >= smallestSizeGroup
	 && getSizeGroup(this->height) >= smallestSizeGroup
	 && this->stride == this->width * pixelSize) {
		// Initialize each mip bin to show the original image
		this->removePyramidStructure();
	}
};

Color4xU8 ImageRgbaU8Impl::packRgba(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) const {
	return Color4xU8(this->packOrder.packRgba(red, green, blue, alpha));
}

Color4xU8 ImageRgbaU8Impl::packRgba(ColorRgbaI32 color) const {
	return Color4xU8(this->packOrder.packRgba(color.red, color.green, color.blue, color.alpha));
}

ColorRgbaI32 ImageRgbaU8Impl::unpackRgba(Color4xU8 rgba, const PackOrder& order) {
	return ColorRgbaI32(
	  getRed(rgba.packed, order),
	  getGreen(rgba.packed, order),
	  getBlue(rgba.packed, order),
	  getAlpha(rgba.packed, order)
	);
}

ColorRgbaI32 ImageRgbaU8Impl::unpackRgba(Color4xU8 rgba) const {
	return unpackRgba(rgba, this->packOrder);
}

Color4xU8 ImageRgbaU8Impl::packRgb(uint8_t red, uint8_t green, uint8_t blue) const {
	return Color4xU8(this->packOrder.packRgba(red, green, blue, 255));
}

Color4xU8 ImageRgbaU8Impl::packRgb(ColorRgbI32 color) const {
	return Color4xU8(this->packOrder.packRgba(color.red, color.green, color.blue, 255));
}

ColorRgbI32 ImageRgbaU8Impl::unpackRgb(Color4xU8 rgb, const PackOrder& order) {
	return ColorRgbI32(
	  getRed(rgb.packed, order),
	  getGreen(rgb.packed, order),
	  getBlue(rgb.packed, order)
	);
}

ColorRgbI32 ImageRgbaU8Impl::unpackRgb(Color4xU8 rgb) const {
	return unpackRgb(rgb, this->packOrder);
}

