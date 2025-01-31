
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

#ifndef DFPSR_TEXTURE_TYPES
#define DFPSR_TEXTURE_TYPES

#include "Image.h"
#include "../../base/noSimd.h" // Scalar versions of the SIMD functions for creating template functions both with and without SIMD.
#include "../../base/DsrTraits.h" // Scalar versions of the SIMD functions for creating template functions both with and without SIMD.
#include "../math/scalar.h"

namespace dsr {

// MIP is a latin acronym "multum in parvo" meaning much in little.
static const uint32_t DSR_MIP_LEVEL_COUNT = 16;

// Mip index 0 is full resolution.
// Mip index 1 is half resolution.
// Mip index 2 is quarter resolution.
// ...
struct Texture {
	Buffer impl_buffer;
	// Base-two logarithms of the highest resolution.
	uint32_t impl_log2width = 0;
	uint32_t impl_log2height = 0;
	// Mip level indices from 0 to impl_maxMipLevel.
	uint32_t impl_maxMipLevel = 0;
	// Number of pixels before the largest mip level.
	uint32_t impl_startOffset = 0;
	uint32_t impl_maxLevelMask = 0;
	// Tiling of unsigned pixel coordinates using bit masks.
	uint32_t impl_minWidthOrMask = 0;
	uint32_t impl_minHeightOrMask = 0;
	uint32_t impl_maxWidthAndMask = 0;
	uint32_t impl_maxHeightAndMask = 0;
	// Maximum dimensions for calculating mip level.
	float impl_floatMaxWidth = 0.0f;
	float impl_floatMaxHeight = 0.0f;
	// What each pixel contains.
	uint8_t impl_pixelFormat = 0;
	Texture() {}
	// TODO: Allow creating a single layer from an existing pixel buffer, which must be free from padding.
	//       If not using multi-threading to write to an image, one can use less than a cache line for alignment.
	//       Store a bit in image saying if the image is a thread-safe write target with cache aligned rows.
	Texture(uint32_t log2width, uint32_t log2height, uint32_t maxMipLevel, PixelFormat format, uint32_t pixelSize)
	: impl_log2width(log2width), impl_log2height(log2height), impl_maxMipLevel(maxMipLevel), impl_pixelFormat(uint8_t(format)) {
		if (maxMipLevel < 0) maxMipLevel = 0;
		if (maxMipLevel >= DSR_MIP_LEVEL_COUNT) maxMipLevel = DSR_MIP_LEVEL_COUNT - 1;
		if ((int32_t)log2width - maxMipLevel < 0 || (int32_t)log2height - maxMipLevel < 0) {
			// TODO: Indicate failure.
			this->impl_pixelFormat = 0;
		} else {
			uint32_t highestLayerPixelCount = uint32_t(1) << (log2width + log2height);
			uint64_t pixelCount = 0;
			uint32_t levelPixelCount = highestLayerPixelCount;
			for (int32_t level = maxMipLevel; level >= 0; level--) {
				pixelCount = pixelCount | levelPixelCount;
				levelPixelCount = levelPixelCount >> 2;
			}
			if (pixelCount > 4294967296) {
				// TODO: Indicate failure to index pixels using 32-bit gather.
			} else {
				this->impl_startOffset = (uint32_t)pixelCount & ~highestLayerPixelCount;
				this->impl_maxLevelMask = highestLayerPixelCount - 1;
				this->impl_minWidthOrMask = (uint32_t(1) << (log2width - maxMipLevel)) - 1;
				this->impl_minHeightOrMask = (uint32_t(1) << (log2height - maxMipLevel)) - 1;
				this->impl_maxWidthAndMask = (uint32_t(1) << log2width) - 1;
				this->impl_maxHeightAndMask = (uint32_t(1) << log2height) - 1;
				this->impl_floatMaxWidth = float((uint32_t(1) << log2width));
				this->impl_floatMaxHeight = float((uint32_t(1) << log2height));
				this->impl_buffer = buffer_create((uint32_t)pixelCount * pixelSize);
			}
		}
	}
};

struct TextureRgbaU8 : public Texture {
	TextureRgbaU8() {}
	TextureRgbaU8(uint32_t log2width, uint32_t log2height, uint32_t maxMipLevel = DSR_MIP_LEVEL_COUNT - 1)
	: Texture(log2width, log2height, min(log2width, log2height, maxMipLevel), PixelFormat::RgbaU8, sizeof(uint32_t)) {}
};

}

#endif
