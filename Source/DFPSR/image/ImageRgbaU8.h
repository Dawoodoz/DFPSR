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

#ifndef DFPSR_IMAGE_RGBA_U8
#define DFPSR_IMAGE_RGBA_U8

#include "Color.h"
#include "Image.h"
#include "ImageU8.h"

namespace dsr {

// TODO: Reallocate the image's buffer, so that the pyramid images are placed into the same allocation.
//       This allow reading a texture with multiple mip levels using different 32-bit offsets in the same SIMD vector holding multiple groups of 2x2 pixels.
// TODO: Adapt how far down to go in mip resolutions based on DSR_DEFAULT_ALIGNMENT, so that no mip level is padded in memory.
//       This is needed so that the stride can be calculated using bit shifting from the mip level.
//       The visual appearance may differ between SIMD lengths for low resolution textures, but not between computers running the same executable.
// TODO: Store the smallest layer first in memory, so that the offset is a multiple of the smallest size following a pre-determined bit pattern.
//       If one s is the number of pixels in the smallest mip layer, then the size of each layer n equals s * 2^n.
//       The offset in s units is then the sum of all previous unpadded dimensions.
//         0000000000000000 0
//         0000000000000001 1
//         0000000000000101 5
//         0000000000010101 21
//         0000000001010101 85
//         0000000101010101 341
//         0000010101010101 1365
//         0001010101010101 5461
//         0101010101010101 21845
//       Then one can start with the offset to the largest mip layer in pixels or bytes as the initial mask and then mask out initial ones using a mask directly from the MIP calculation.
//         0000000000000000 4x2 pixels at offset 0
//         0000000000001000 8x4 pixels at offset 8
//         0000000000101000 16x32 pixels at offset 40
//         0000000010101000 32x64 pixels at offset 168
//         0000001010101000 64x128 pixels at offset 680
//         0000101010101000 128x256 pixels at offset 2728 (Full unmasked offset leading to the highest mip level)
//       Masks for different visibility.
//         0000000000000011 Very far away or seen from the side
//         0000000000111111 Far away or seen from the side
//         0000001111111111 Normal viewing
//         0011111111111111 Close with many screen pixels per texels.
//       The difficult part is how to generate a good mip level offset mask from the pixel coordinate derivation from groups of 2x2 pixels.
//         The offset is not exactly exponential, so there will be visual tradeoffs between artifacts in this approximation.
//       One could take the texture coordinate offset per pixel as the initial value and
//         then repeat shifting and or masking at power of two offsets to only get ones after the initial one, but this would require many cycles.
//           Pixels per texel in full resolution times full resolution offset:
//             00000000000010101000110110011001
//           Mip offset mask:
//             00000000000011111111111111111111
//           Full resolution mip offset:
//             00000000001010101010101010000000
//           Final mip offset containing half width and height:
//             00000000000010101010101010000000

// Pointing to the parent image using raw pointers for fast rendering. May not exceed the lifetime of the parent image!
struct TextureRgbaLayer {
	const uint8_t *data = 0;
	int32_t strideShift = 0;
	uint32_t widthMask = 0, heightMask = 0;
	int32_t width = 0, height = 0;
	float subWidth = 0.0f, subHeight = 0.0f; // TODO: Better names?
	float halfPixelOffsetU = 0.0f, halfPixelOffsetV = 0.0f;
	TextureRgbaLayer();
	TextureRgbaLayer(const uint8_t *data, int32_t width, int32_t height);
	// Can it be sampled as a texture
	bool exists() const { return this->data != nullptr; }
};

#define MIP_BIN_COUNT 5

// Pointing to the parent image using raw pointers for fast rendering. Not not separate from the image!
struct TextureRgba {
	Buffer pyramidBuffer; // Storing the smaller mip levels
	TextureRgbaLayer mips[MIP_BIN_COUNT]; // Pointing to all mip levels including the original image
	// Can it be sampled as a texture
	bool exists() const { return this->mips[0].exists(); }
	// Does it have a mip pyramid generated for smoother sampling
	bool hasMipBuffer() const { return this->pyramidBuffer.get() != nullptr; }
};

class ImageRgbaU8Impl : public ImageImpl {
public:
	static const int32_t channelCount = 4;
	static const int32_t pixelSize = channelCount;
	PackOrder packOrder;
	// Macro defined functions
	IMAGE_DECLARATION(ImageRgbaU8Impl, 4, Color4xU8, uint8_t);
	// Constructors
	ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight, int32_t newStride, Buffer buffer, intptr_t startOffset, const PackOrder &packOrder);
	ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight, int32_t alignment);
	// Native canvas constructor
	ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight, PackOrderIndex packOrderIndex, int32_t alignment);
	// Fast reading
	TextureRgba texture; // The texture view
	void initializeRgbaImage(); // Points to level 0 from all bins to allow rendering
	void generatePyramid(); // Fills the following bins with smaller images
	void removePyramid();
	bool isTexture() const;
	static bool isTexture(const ImageRgbaU8Impl* image); // Null cannot be sampled as a texture
public:
	// Conversion to monochrome by extracting a channel
	ImageU8Impl getChannel(int32_t channelIndex) const;
	// Clone the image without padding or return the same instance if there is no padding
	// TODO: Return the unaligned image type, which is incompatible with SIMD operations
	ImageRgbaU8Impl getWithoutPadding() const;
	// Packs/unpacks the channels of an RGBA color in an unsigned 32-bit integer
	Color4xU8 packRgba(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) const;
	Color4xU8 packRgba(ColorRgbaI32 rgba) const;
	static ColorRgbaI32 unpackRgba(Color4xU8 rgba, const PackOrder& order);
	ColorRgbaI32 unpackRgba(Color4xU8 rgba) const;
	// Packs/unpacks the channels of an RGB color in an unsigned 32-bit integer
	Color4xU8 packRgb(uint8_t red, uint8_t green, uint8_t blue) const;
	Color4xU8 packRgb(ColorRgbI32 rgb) const;
	static ColorRgbI32 unpackRgb(Color4xU8 rgb, const PackOrder& order);
	ColorRgbI32 unpackRgb(Color4xU8 rgb) const;
};

}

#endif
