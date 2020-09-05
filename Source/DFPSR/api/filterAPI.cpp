
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

#define DFPSR_INTERNAL_ACCESS

#include <cassert>
#include "imageAPI.h"
#include "filterAPI.h"
#include "../image/draw.h"
#include "../image/PackOrder.h"
#include "../image/internal/imageTemplate.h"
#include "../image/internal/imageInternal.h"

using namespace dsr;


// -------------------------------- Image generation and filtering --------------------------------


static void mapRgbaU8(ImageRgbaU8Impl& target, const ImageGenRgbaU8& lambda, int startX, int startY) {
	const int targetWidth = target.width;
	const int targetHeight = target.height;
	const int targetStride = target.stride;
	SafePointer<Color4xU8> targetRow = imageInternal::getSafeData<Color4xU8>(target);
	for (int y = startY; y < targetHeight + startY; y++) {
		SafePointer<Color4xU8> targetPixel = targetRow;
		for (int x = startX; x < targetWidth + startX; x++) {
			*targetPixel = target.packRgba(lambda(x, y).saturate());
			targetPixel += 1;
		}
		targetRow.increaseBytes(targetStride);
	}
}
void dsr::filter_mapRgbaU8(ImageRgbaU8 target, const ImageGenRgbaU8& lambda, int startX, int startY) {
	if (target.get() != nullptr) {
		mapRgbaU8(*target, lambda, startX, startY);
	}
}
OrderedImageRgbaU8 dsr::filter_generateRgbaU8(int width, int height, const ImageGenRgbaU8& lambda, int startX, int startY) {
	OrderedImageRgbaU8 result = image_create_RgbaU8(width, height);
	filter_mapRgbaU8(result, lambda, startX, startY);
	return result;
}

template <typename IMAGE_TYPE, typename PIXEL_TYPE, int MIN_VALUE, int MAX_VALUE>
static void mapMonochrome(IMAGE_TYPE& target, const ImageGenI32& lambda, int startX, int startY) {
	const int targetWidth = target.width;
	const int targetHeight = target.height;
	const int targetStride = target.stride;
	SafePointer<PIXEL_TYPE> targetRow = imageInternal::getSafeData<PIXEL_TYPE>(target);
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
void dsr::filter_mapU8(ImageU8 target, const ImageGenI32& lambda, int startX, int startY) {
	if (target.get() != nullptr) {
		mapMonochrome<ImageU8Impl, uint8_t, 0, 255>(*target, lambda, startX, startY);
	}
}
AlignedImageU8 dsr::filter_generateU8(int width, int height, const ImageGenI32& lambda, int startX, int startY) {
	AlignedImageU8 result = image_create_U8(width, height);
	filter_mapU8(result, lambda, startX, startY);
	return result;
}
void dsr::filter_mapU16(ImageU16 target, const ImageGenI32& lambda, int startX, int startY) {
	if (target.get() != nullptr) {
		mapMonochrome<ImageU16Impl, uint16_t, 0, 65535>(*target, lambda, startX, startY);
	}
}
AlignedImageU16 dsr::filter_generateU16(int width, int height, const ImageGenI32& lambda, int startX, int startY) {
	AlignedImageU16 result = image_create_U16(width, height);
	filter_mapU16(result, lambda, startX, startY);
	return result;
}

static void mapF32(ImageF32Impl& target, const ImageGenF32& lambda, int startX, int startY) {
	const int targetWidth = target.width;
	const int targetHeight = target.height;
	const int targetStride = target.stride;
	SafePointer<float> targetRow = imageInternal::getSafeData<float>(target);
	for (int y = startY; y < targetHeight + startY; y++) {
		SafePointer<float> targetPixel = targetRow;
		for (int x = startX; x < targetWidth + startX; x++) {
			*targetPixel = lambda(x, y);
			targetPixel += 1;
		}
		targetRow.increaseBytes(targetStride);
	}
}
void dsr::filter_mapF32(ImageF32 target, const ImageGenF32& lambda, int startX, int startY) {
	if (target.get() != nullptr) {
		mapF32(*target, lambda, startX, startY);
	}
}
AlignedImageF32 dsr::filter_generateF32(int width, int height, const ImageGenF32& lambda, int startX, int startY) {
	AlignedImageF32 result = image_create_F32(width, height);
	filter_mapF32(result, lambda, startX, startY);
	return result;
}


// -------------------------------- Resize --------------------------------


static OrderedImageRgbaU8 resizeToRef(const ImageRgbaU8Impl& source, Sampler interpolation, int32_t newWidth, int32_t newHeight) {
	OrderedImageRgbaU8 resultImage = image_create_RgbaU8(newWidth, newHeight);
	imageImpl_resizeToTarget(*resultImage, source, interpolation == Sampler::Linear);
	return resultImage;
}

OrderedImageRgbaU8 dsr::filter_resize(ImageRgbaU8 source, Sampler interpolation, int32_t newWidth, int32_t newHeight) {
	if (source.get() != nullptr) {
		return resizeToRef(*source, interpolation, newWidth, newHeight);
	} else {
		return OrderedImageRgbaU8(); // Null gives null
	}
}

void dsr::filter_blockMagnify(ImageRgbaU8 target, const ImageRgbaU8& source, int pixelWidth, int pixelHeight) {
	if (target.get() != nullptr && source.get() != nullptr) {
		imageImpl_blockMagnify(*target, *source, pixelWidth, pixelHeight);
	}
}

