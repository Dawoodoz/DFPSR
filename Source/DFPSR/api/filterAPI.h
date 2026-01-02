
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

#ifndef DFPSR_API_FILTER
#define DFPSR_API_FILTER

#include "../implementation/image/Image.h"
#include <functional>

namespace dsr {

// Sampling modes
	enum class Sampler {
		Nearest, // Taking the nearest value to create square pixels.
		Linear   // Taking a linear interpolation of the nearest pixels.
	};

// Image resizing
	// Create a stretched version of the source image with the given dimensions and default RGBA pack order.
	OrderedImageRgbaU8 filter_resize(const ImageRgbaU8 &source, Sampler interpolation, int32_t newWidth, int32_t newHeight);
	AlignedImageU8     filter_resize(const ImageU8 &source,     Sampler interpolation, int32_t newWidth, int32_t newHeight);
	// The nearest-neighbor resize used for up-scaling the window canvas.
	//   The source image is scaled by pixelWidth and pixelHeight from the upper left corner.
	//   If source is too small, transparent black pixels (0, 0, 0, 0) fills the outside.
	//   If source is too large, partial pixels will be cropped away completely and replaced by the black border.
	//   Letting the images have the same pack order and be aligned to 16-bytes will increase speed.
	void filter_blockMagnify(const ImageRgbaU8 &target, const ImageRgbaU8 &source, int32_t pixelWidth, int32_t pixelHeight);

// Image generation and filtering
//   Create images from Lambda expressions when speed is not critical.
//     Capture images within [] and sample pixels from them using image_readPixel_border, image_readPixel_clamp and image_readPixel_tile.
	// Lambda expressions for generating integer images.
	using ImageGenRgbaU8 = std::function<ColorRgbaI32(int32_t x, int32_t y)>;
	using ImageGenI32 = std::function<int32_t(int32_t x, int32_t y)>; // Used for U8 and U16 images using different saturations.
	using ImageGenF32 = std::function<float(int32_t x, int32_t y)>;
	// In-place image generation to an existing image.
	//   The pixel at the upper left corner gets (startX, startY) as x and y arguments to the function.
	void filter_mapRgbaU8(const ImageRgbaU8 &target, const ImageGenRgbaU8& lambda, int32_t startX = 0, int32_t startY = 0);
	void filter_mapU8(const ImageU8 &target, const ImageGenI32& lambda, int32_t startX = 0, int32_t startY = 0);
	void filter_mapU16(const ImageU16 &target, const ImageGenI32& lambda, int32_t startX = 0, int32_t startY = 0);
	void filter_mapF32(const ImageF32 &target, const ImageGenF32& lambda, int32_t startX = 0, int32_t startY = 0);
	// A simpler image generation that constructs the image as a result.
	// Example:
	//     int32_t width = 64;
	//     int32_t height = 64;
	//     ImageRgbaU8 fadeImage = filter_generateRgbaU8(width, height, [](int32_t x, int32_t y)->ColorRgbaI32 {
	//         return ColorRgbaI32(x * 4, y * 4, 0, 255);
	//     });
	//     ImageRgbaU8 brighterImage = filter_generateRgbaU8(width, height, [fadeImage](int32_t x, int32_t y)->ColorRgbaI32 {
	//	       ColorRgbaI32 source = image_readPixel_clamp(fadeImage, x, y);
	//	       return ColorRgbaI32(source.red * 2, source.green * 2, source.blue * 2, source.alpha);
	//     });
	OrderedImageRgbaU8 filter_generateRgbaU8(int32_t width, int32_t height, const ImageGenRgbaU8& lambda, int32_t startX = 0, int32_t startY = 0);
	AlignedImageU8 filter_generateU8(int32_t width, int32_t height, const ImageGenI32& lambda, int32_t startX = 0, int32_t startY = 0);
	AlignedImageU16 filter_generateU16(int32_t width, int32_t height, const ImageGenI32& lambda, int32_t startX = 0, int32_t startY = 0);
	AlignedImageF32 filter_generateF32(int32_t width, int32_t height, const ImageGenF32& lambda, int32_t startX = 0, int32_t startY = 0);

}

#endif

