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
	std::shared_ptr<Buffer> pyramidBuffer; // Storing the smaller mip levels
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
	ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight, int32_t newStride, std::shared_ptr<Buffer> buffer, intptr_t startOffset, PackOrder packOrder);
	ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight, int32_t alignment = 16);
	// Native canvas constructor
	ImageRgbaU8Impl(int32_t newWidth, int32_t newHeight, PackOrderIndex packOrderIndex);
	// Fast reading
	TextureRgba texture; // The texture view
	void initializeRgbaImage(); // Points to level 0 from all bins to allow rendering
	void generatePyramid(); // Fills the following bins with smaller images
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

