
// zlib open source license
//
// Copyright (c) 2025 David Forsgren Piuva
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

#include "textureAPI.h"
#include "imageAPI.h"
#include "filterAPI.h"
			#include "drawAPI.h"

namespace dsr {

static int findLog2Size(uint32_t size) {
	static const uint32_t maxLog2Size = 15; // 32768 pixels
	for (uint32_t log2Size = 0; log2Size < maxLog2Size; log2Size++) {
		if ((uint32_t(1u) << log2Size) >= size) {
			// Found a size that is large enough.
			return log2Size;
		}
	}
	// Reached the upper limit.
	return maxLog2Size;
}

// TODO: Optimize using addition and SafePointer.
static void downsample(const TextureRgbaU8 &texture, uint32_t targetLevel) {
	uint32_t sourceLevel = targetLevel - 1;
	uint32_t targetWidth = texture_getWidth(texture, targetLevel);
	uint32_t targetHeight = texture_getHeight(texture, targetLevel);
	for (uint32_t y = 0; y < targetHeight; y++) {
		for (uint32_t x = 0; x < targetWidth; x++) {
			uint32_t upperLeft  = texture_readPixel(texture, x * 2    , y * 2    , sourceLevel);
			uint32_t upperRight = texture_readPixel(texture, x * 2 + 1, y * 2    , sourceLevel);
			uint32_t lowerLeft  = texture_readPixel(texture, x * 2    , y * 2 + 1, sourceLevel);
			uint32_t lowerRight = texture_readPixel(texture, x * 2 + 1, y * 2 + 1, sourceLevel);
			uint32_t mixedColor = packOrder_packBytes(
			  (packOrder_getRed  (upperLeft) + packOrder_getRed  (upperRight) + packOrder_getRed  (lowerLeft) + packOrder_getRed  (lowerRight)) / 4,
			  (packOrder_getGreen(upperLeft) + packOrder_getGreen(upperRight) + packOrder_getGreen(lowerLeft) + packOrder_getGreen(lowerRight)) / 4,
			  (packOrder_getBlue (upperLeft) + packOrder_getBlue (upperRight) + packOrder_getBlue (lowerLeft) + packOrder_getBlue (lowerRight)) / 4,
			  (packOrder_getAlpha(upperLeft) + packOrder_getAlpha(upperRight) + packOrder_getAlpha(lowerLeft) + packOrder_getAlpha(lowerRight)) / 4
			);
			texture_writePixel(texture, x, y, targetLevel, mixedColor);
		}
	}
}

TextureRgbaU8 texture_create_RgbaU8(int32_t width, int32_t height, int32_t resolutions) {
	if (resolutions < 1) {
		throwError(U"Tried to create a texture without any resolutions stored, which would be empty!\n");
		return TextureRgbaU8();
	} else if (width < 1 || height < 1) {
		throwError(U"Tried to create a texture of ", width, U" x ", height, U" pixels, which would be empty!\n");
		return TextureRgbaU8();
	} else if (width > 32768 || height > 32768) {
		throwError(U"Tried to create a texture of ", width, U" x ", height, U" pixels, which exceeds the maximum texture dimensions of 32768 x 32768 pixels!\n");
		return TextureRgbaU8();
	} else {
		return TextureRgbaU8(findLog2Size(width), findLog2Size(height), resolutions - 1);
	}
}

static uint64_t testCounter = 0;

void texture_generatePyramid(const TextureRgbaU8& texture) {
	uint32_t mipLevelCount = texture_getMipLevelCount(texture);
	for (uint32_t targetLevel = 1; targetLevel < mipLevelCount; targetLevel++) {
		downsample(texture, targetLevel);
	}
}

TextureRgbaU8 texture_create_RgbaU8(const ImageRgbaU8& image, int32_t resolutions) {
	if (!image_exists(image)) {
		// An empty image returns an empty pyramid.
		return TextureRgbaU8();
	} else {
		// Allocate a pyramid image.
		TextureRgbaU8 result = texture_create_RgbaU8(image_getWidth(image), image_getHeight(image), resolutions);
		uint32_t width = texture_getMaxWidth(result);
		uint32_t height = texture_getMaxHeight(result);
		// Create an image of the same size as the largest resolution.
		OrderedImageRgbaU8 resized = filter_resize(image, Sampler::Linear, width, height);
		testCounter++;
		// Copy from the resized image to the highest resolution in the pyramid.
		for (uint32_t y = 0; y < height; y++) {
			SafePointer<uint32_t> source = image_getSafePointer(resized, y);
			SafePointer<uint32_t> target = texture_getSafePointer(result, 0u, y);
			safeMemoryCopy(target, source, width * sizeof(uint32_t));
		}
		texture_generatePyramid(result);
		return result;
	}
}

ImageRgbaU8 texture_getMipLevelImage(const TextureRgbaU8& texture, int32_t mipLevel) {
	if (!texture_exists(texture)) {
		throwError(U"Can not get a mip level as an image from a texture that does not exist!\n");
		return ImageRgbaU8();
	} else if (mipLevel < 0 || mipLevel > texture_getSmallestMipLevel(texture)) {
		throwError(U"Can not get a non-existing mip level at index ", mipLevel, U" from a texture with layers 0..", texture_getSmallestMipLevel(texture), U"!\n");
		throwError(U"");
		return ImageRgbaU8();
	} else {
		return ImageRgbaU8(texture.impl_buffer, texture_getPixelOffsetToLayer<false, uint32_t>(texture, mipLevel), texture_getWidth(texture, mipLevel), texture_getHeight(texture, mipLevel), texture_getWidth(texture, mipLevel), PackOrderIndex::RGBA);
	}
}

}
