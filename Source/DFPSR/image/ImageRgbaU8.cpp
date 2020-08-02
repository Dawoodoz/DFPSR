// zlib open source license
//
// Copyright (c) 2017 to 2019 David Forsgren Piuva
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
#include <algorithm>

using namespace dsr;

IMAGE_DEFINITION(ImageRgbaU8Impl, 4, Color4xU8, uint8_t);

ImageRgbaU8Impl::ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight, int32_t newStride, std::shared_ptr<Buffer> buffer, intptr_t startOffset, PackOrder packOrder) :
  ImageImpl(newWidth, newHeight, newStride, sizeof(Color4xU8), buffer, startOffset), packOrder(packOrder) {
	assert(buffer->size - startOffset >= imageInternal::getUsedBytes(this));
	this->initializeRgbaImage();
}

ImageRgbaU8Impl::ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight, int32_t alignment) :
  ImageImpl(newWidth, newHeight, roundUp(newWidth * sizeof(Color4xU8), alignment), sizeof(Color4xU8)) {
	this->initializeRgbaImage();
}

// Native canvas constructor
ImageRgbaU8Impl::ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight, PackOrderIndex packOrderIndex) :
  ImageImpl(newWidth, newHeight, roundUp(newWidth * sizeof(Color4xU8), 16), sizeof(Color4xU8)) {
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
	if (this->stride == this->width * this->pixelSize) {
		// No padding
		return *this;
	} else {
		// Copy each row without padding
		ImageRgbaU8Impl result(this->width, this->height, 1);
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

static int32_t getSizeGroup(int32_t size) {
	int32_t group = -1;
	if (size == 1) {
		group = 0; // Too small for 16-byte alignment!
	} else if (size == 2) {
		group = 1; // Too small for 16-byte alignment!
	} else if (size == 4) {
		group = 2; // Smallest allowed texture dimension
	} else if (size == 8) {
		group = 3;
	} else if (size == 16) {
		group = 4;
	} else if (size == 32) {
		group = 5;
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

static int32_t getPyramidSize(int32_t width, int32_t height, int32_t pixelSize, int32_t levels) {
	uint32_t result = 0;
	uint32_t byteCount = width * height * pixelSize;
	for (int32_t l = 0; l < levels; l++) {
		result += byteCount; // Add image size to pyramid size
		byteCount = byteCount >> 2; // Divide size by 4
	}
	return (int32_t)result;
}

static void downScaleByTwo(SafePointer<uint8_t> targetData, const SafePointer<uint8_t> sourceData, int32_t targetWidth, int32_t targetHeight, int32_t pixelSize, int32_t targetStride) {
	int32_t sourceStride = targetStride * 2;
	int32_t doubleSourceStride = sourceStride * 2;
	SafePointer<uint8_t> targetRow = targetData;
	const SafePointer<uint8_t> sourceRow = sourceData;
	for (int32_t y = 0; y < targetHeight; y++) {
		const SafePointer<uint8_t> sourcePixel = sourceRow;
		SafePointer<uint8_t> targetPixel = targetRow;
		for (int32_t x = 0; x < targetWidth; x++) {
			// TODO: Use pariwise and vector average functions for fixed channel counts (SSE has _mm_avg_epu8 for vector average)
			for (int32_t c = 0; c < pixelSize; c++) {
				uint8_t value = (uint8_t)((
				    (uint16_t)(*sourcePixel)
				  + (uint16_t)(*(sourcePixel + pixelSize))
				  + (uint16_t)(*(sourcePixel + sourceStride))
				  + (uint16_t)(*(sourcePixel + sourceStride + pixelSize))) / 4);
				*targetPixel = value;
				targetPixel += 1;
				sourcePixel += 1;
			}
			sourcePixel += pixelSize;
		}
		targetRow += targetStride;
		sourceRow += doubleSourceStride;
	}
}

TextureRgbaLayer::TextureRgbaLayer() {}

TextureRgbaLayer::TextureRgbaLayer(const uint8_t *data, int32_t width, int32_t height) :
  data(data),
  strideShift(getSizeGroup(width) + 2),
  widthMask(width - 1),
  heightMask(height - 1),
  width(width),
  height(height),
  subWidth(width * 256),
  subHeight(height * 256),
  halfPixelOffsetU(1.0f - (0.5f / width)),
  halfPixelOffsetV(1.0f - (0.5f / height)) {}

void ImageRgbaU8Impl::generatePyramid() {
	if (!this->isTexture()) {
		if (this->width < 4 || this->height < 4) {
			printText("Cannot generate a pyramid from an image smaller than 4x4 pixels.\n");
		} else if (this->width > 16384 || this->height > 16384) {
			printText("Cannot generate a pyramid from an image larger than 16384x16384 pixels.\n");
		} else if (getSizeGroup(this->width) == -1 || getSizeGroup(this->height) == -1) {
			printText("Cannot generate a pyramid from image dimensions that are not powers of two.\n");
		} else if (this->stride > this->width * pixelSize) {
			printText("Cannot generate a pyramid from an image that contains padding.\n");
		} else if (this->stride < this->width * pixelSize) {
			printText("Cannot generate a pyramid from an image with corrupted stride.\n");
		} else {
			printText("Cannot generate a pyramid from an image that has not been initialized correctly.\n");
		}
	} else {
		int32_t pixelSize = this->pixelSize;
		int32_t mipmaps = std::min(std::max(getSizeGroup(std::min(this->width, this->height)) - 1, 1), MIP_BIN_COUNT);
		if (!this->texture.hasMipBuffer()) {
			this->texture.pyramidBuffer = Buffer::create(getPyramidSize(this->width / 2, this->height / 2, pixelSize, mipmaps - 1));
		}
		// Point to the image's original buffer in mip level 0
		SafePointer<uint8_t> currentStart = imageInternal::getSafeData<uint8_t>(*this);
		int32_t currentWidth = this->width;
		int32_t currentHeight = this->height;
		this->texture.mips[0] = TextureRgbaLayer(currentStart.getUnsafe(), currentWidth, currentHeight);
		// Create smaller pyramid images in the extra buffer
		SafePointer<uint8_t> previousStart = currentStart;
		currentStart = this->texture.pyramidBuffer->getSafeData<uint8_t>("Pyramid generation target");
		for (int32_t m = 1; m < mipmaps; m++) {
			currentWidth /= 2;
			currentHeight /= 2;
			this->texture.mips[m] = TextureRgbaLayer(currentStart.getUnsafe(), currentWidth, currentHeight);
			int32_t size = currentWidth * currentHeight * pixelSize;
			// In-place downscaling by two.
			downScaleByTwo(currentStart, previousStart, currentWidth, currentHeight, pixelSize, currentWidth * pixelSize);
			previousStart = currentStart;
			currentStart.increaseBytes(size);
		}
		// Fill unused mip levels with duplicates of the last mip level
		for (int32_t m = mipmaps; m < MIP_BIN_COUNT; m++) {
			this->texture.mips[m] = this->texture.mips[m - 1];
		}
	}
}

void ImageRgbaU8Impl::removePyramid() {
	// Only try to remove if it has a pyramid
	if (this->texture.pyramidBuffer.get() != nullptr) {
		// Remove the pyramid's buffer
		this->texture.pyramidBuffer = std::shared_ptr<Buffer>();
		// Re-initialize
		for (int32_t m = 0; m < MIP_BIN_COUNT; m++) {
			this->texture.mips[m] = TextureRgbaLayer(imageInternal::getSafeData<uint8_t>(*this).getUnsafe(), this->width, this->height);
		}
	}
}

void ImageRgbaU8Impl::initializeRgbaImage() {
	// If the image fills the criterias of a texture
	if (getSizeGroup(this->width) >= 2
	 && getSizeGroup(this->height) >= 2
	 && this->stride == this->width * this->pixelSize) {
		// Initialize each mip bin to show the original image
		for (int32_t m = 0; m < MIP_BIN_COUNT; m++) {
			this->texture.mips[m] = TextureRgbaLayer(imageInternal::getSafeData<uint8_t>(*this).getUnsafe(), this->width, this->height);
		}
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

