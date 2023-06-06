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

#ifndef DFPSR_IMAGE_RGBA_U8
#define DFPSR_IMAGE_RGBA_U8

#include "Color.h"
#include "Image.h"
#include "ImageU8.h"

namespace dsr {

// TODO: Figure out how to handle very small textures where the full resolution is smaller than the smallest allowed MIP level.
//       Larger SIMD vectors will change the smallest allowed textures if the bit pattern is used, which may be inconsistent if it keeps growing.
//       Having 32x32 pixels as the minimum size would allow using up to 1024-bit SIMD without a problem.
// TODO: Reallocate the image's buffer, so that the pyramid images are placed into the same allocation.
//       This allow reading a texture with multiple mip levels using different 32-bit offsets in the same SIMD vector holding multiple groups of 2x2 pixels.
// TODO: Adapt how far down to go in mip resolutions based on DSR_DEFAULT_ALIGNMENT, so that no mip level is padded in memory.
//       This is needed so that the stride can be calculated using bit shifting from the mip level.
//       The visual appearance may differ between SIMD lengths for low resolution textures, but not between computers running the same executable.
// TODO: Begin by replacing the lookup table for pyramid layers with template inline functions, because figuring out how to get start offset and stride consistently may take time.
//       Pixel loops will later look up the masks once and store it in SIMD vectors to avoid fetching it from memory multiple times from potential memory aliasing.
// IDEA: Keep the same order of mip layers, but mask out offset bits from the right side.
//       When the most significant bit is masked out, it jumps to the full resoultion image at offset zero.
//       Offsets
//         00000000000000000000000000000000 Full resolution of 64x64
//         00000000000000000000010000000000 Half resolution of 32x32
//         00000000000000000000010100000000 Quarter resolution of 16x16
//         00000000000000000000010101000000 Low resolution of 8x8
//         00000000000000000000010101010000 Lowest resolution of 4x4
//       Power of 4 offset masks
//         11111111111111111100000000000000 Show at most 16384 pixels (clamped to full resolution because no more bits are masked out)
//         11111111111111111111000000000000 Show at most 4096 pixels (full resolution for the image)
//         11111111111111111111110000000000 Show at most 1024 pixels
//         11111111111111111111111100000000 Show at most 256 pixels
//         11111111111111111111111111000000 Show at most 64 pixels
//         11111111111111111111111111110000 Show at most 16 pixels
// PROBLEMS:
//   * How can stride be calculated in the same way as the start offset?
//     - Consistently in base two, not using the base 4 mask.
//     - Limited to the range of available resolutions, not going to stride 512 when the full resolution stride is 256.
//   * What about the width and height masks, can they reuse the same bit masking to avoid looking up data with scalar operations?
//   * What should be done about very small textures?
//     Automatically scale them up to the minimum resolution and leave the original image in the middle of the buffer?
//     Change minimum size requirements?
//       This would be the simplest approach and nobody would want their textures up-scaled anyway if one can easily redraw images in a higher resolution.

// Pointing to the parent image using raw pointers for fast rendering. May not exceed the lifetime of the parent image!
struct TextureRgbaLayer {
	SafePointer<uint32_t> data;                                         // Replace with the start offset
	int32_t strideShift = 0;                                            // Subtract one per layer 
	uint32_t widthMask = 0, heightMask = 0;                             // Shift one bit right per layer
	int32_t width = 0, height = 0;                                      // Shift one bit right per layer
	float subWidth = 0.0f, subHeight = 0.0f;                            // Try to use integers, so that these can be shifted
	TextureRgbaLayer();
	TextureRgbaLayer(SafePointer<uint32_t> data, int32_t width, int32_t height);
	// Can it be sampled as a texture
	bool exists() const { return this->data.getUnsafe() != nullptr; }
};

// TODO: Try to replace with generated bit masks from inline functions.
#define MIP_BIN_COUNT 5

// Pointing to the parent image using raw pointers for fast rendering. Do not separate from the image!
struct TextureRgba {
	// TODO: Remove the array, so that any number of layers can be contained by calculating the masks and offsets.
	TextureRgbaLayer mips[MIP_BIN_COUNT]; // Pointing to all mip levels including the original image
	int32_t layerCount = 0; // 0 Means that there are no pointers, 1 means that you have a pyramid but only one layer.
	// Can it be sampled as a texture
	bool exists() const { return this->mips[0].exists(); }
	// Does it have a mip pyramid generated for smoother sampling
	// TODO: Rename once there is no separate MIP buffer, just a single pyramid buffer.
	bool hasMipBuffer() const { return this->layerCount != 0; }
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
	// The texture view for fast reading
	TextureRgba texture;
	// Points to level 0 from all bins to allow rendering
	void initializeRgbaImage();
	// Resizes the image to valid texture dimensions
	void makeIntoTexture();
	void generatePyramid(); // Fills the following bins with smaller images
	void removePyramid();
	bool isTexture() const;
	static bool isTexture(const ImageRgbaU8Impl* image); // Null cannot be sampled as a texture
private:
	void generatePyramidStructure(int32_t layerCount);
	void removePyramidStructure();
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
