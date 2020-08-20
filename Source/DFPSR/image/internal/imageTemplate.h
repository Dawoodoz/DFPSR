// zlib open source license
//
// Copyright (c) 2018 to 2019 David Forsgren Piuva
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

#ifndef DFPSR_IMAGE_TEMPLATE
#define DFPSR_IMAGE_TEMPLATE

#include "imageInternal.h"
#include "../../math/scalar.h"
#include "../Image.h"
#include <limits>

namespace dsr {

// TODO: Remove clamped pixel operation
// Each image type must define initializeImage instead of a constructor;
// These macros are used to compile instances of template functions because it's much safer than exposing header defined template classes.
#define IMAGE_DEFINITION(IMAGE_TYPE,CHANNELS,COLOR_TYPE,ELEMENT_TYPE) \
	void IMAGE_TYPE::writePixel(IMAGE_TYPE &image, int32_t x, int32_t y, COLOR_TYPE color) { \
		if (x >= 0 && x < image.width && y >= 0 && y < image.height) { \
			*(COLOR_TYPE*)(buffer_dangerous_getUnsafeData(image.buffer) + image.startOffset + (x * sizeof(COLOR_TYPE)) + (y * image.stride)) = color; \
		} \
	} \
	void IMAGE_TYPE::writePixel_unsafe(IMAGE_TYPE &image, int32_t x, int32_t y, COLOR_TYPE color) { \
		*(COLOR_TYPE*)(buffer_dangerous_getUnsafeData(image.buffer) + image.startOffset + (x * sizeof(COLOR_TYPE)) + (y * image.stride)) = color; \
	} \
	COLOR_TYPE IMAGE_TYPE::readPixel_clamp(const IMAGE_TYPE &image, int32_t x, int32_t y) { \
		if (image.width > 0 && image.height > 0) { \
			if (x < 0) { x = 0; } \
			if (y < 0) { y = 0; } \
			if (x >= image.width) { x = image.width - 1; } \
			if (y >= image.height) { y = image.height - 1; } \
			return *(COLOR_TYPE*)(buffer_dangerous_getUnsafeData(image.buffer) + image.startOffset + (x * sizeof(COLOR_TYPE)) + (y * image.stride)); \
		} else { \
			return COLOR_TYPE(); \
		} \
	} \
	COLOR_TYPE IMAGE_TYPE::readPixel_unsafe(const IMAGE_TYPE &image, int32_t x, int32_t y) { \
		assert(x >= 0 && x < image.width && y >= 0 && y < image.height); \
		return *(COLOR_TYPE*)(buffer_dangerous_getUnsafeData(image.buffer) + image.startOffset + (x * sizeof(COLOR_TYPE)) + (y * image.stride)); \
	}

}

#endif

