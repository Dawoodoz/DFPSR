
// zlib open source license
//
// Copyright (c) 2017 to 2020 David Forsgren Piuva
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

#include "types.h"
#include <functional>

namespace dsr {

// Image resizing
	// The interpolate argument
	//   Bi-linear interoplation is used when true
	//   Nearest neighbor sampling is used when false
	// Create a stretched version of the source image with the given dimensions and default RGBA pack order
	OrderedImageRgbaU8 filter_resize(ImageRgbaU8 source, Sampler interpolation, int32_t newWidth, int32_t newHeight);
	// The nearest-neighbor resize used for up-scaling the window canvas
	//   The source image is scaled by pixelWidth and pixelHeight from the upper left corner
	//   If source is too small, transparent black pixels (0, 0, 0, 0) fills the outside
	//   If source is too large, partial pixels will be cropped away completely and replaced by the black border
	//   Letting the images have the same pack order and be aligned to 16-bytes will increase speed
	void filter_blockMagnify(ImageRgbaU8 target, const ImageRgbaU8& source, int pixelWidth, int pixelHeight);

// Image generation and filtering
//   Create new images from Lambda expressions
//   Useful for pre-generating images for reference implementations, fast prototyping and texture generation
	// Lambda expressions for generating integer images
	using ImageGenRgbaU8 = std::function<ColorRgbaI32(int, int)>;
	using ImageGenI32 = std::function<int32_t(int, int)>; // Used for U8 and U16 images using different saturations
	using ImageGenF32 = std::function<float(int, int)>;
	// In-place image generation to an existing image
	//   The pixel at the upper left corner gets (startX, startY) as x and y arguments to the function
	void filter_mapRgbaU8(ImageRgbaU8 target, const ImageGenRgbaU8& lambda, int startX = 0, int startY = 0);
	void filter_mapU8(ImageU8 target, const ImageGenI32& lambda, int startX = 0, int startY = 0);
	void filter_mapU16(ImageU16 target, const ImageGenI32& lambda, int startX = 0, int startY = 0);
	void filter_mapF32(ImageF32 target, const ImageGenF32& lambda, int startX = 0, int startY = 0);
	// A simpler image generation that constructs the image as a result
	// Example:
	//     int width = 64;
	//     int height = 64;
	//     ImageRgbaU8 fadeImage = filter_generateRgbaU8(width, height, [](int x, int y)->ColorRgbaI32 {
	//         return ColorRgbaI32(x * 4, y * 4, 0, 255);
	//     });
	//     ImageRgbaU8 brighterImage = filter_generateRgbaU8(width, height, [fadeImage](int x, int y)->ColorRgbaI32 {
	//	       ColorRgbaI32 source = image_readPixel_clamp(fadeImage, x, y);
	//	       return ColorRgbaI32(source.red * 2, source.green * 2, source.blue * 2, source.alpha);
	//     });
	OrderedImageRgbaU8 filter_generateRgbaU8(int width, int height, const ImageGenRgbaU8& lambda, int startX = 0, int startY = 0);
	AlignedImageU8 filter_generateU8(int width, int height, const ImageGenI32& lambda, int startX = 0, int startY = 0);
	AlignedImageU16 filter_generateU16(int width, int height, const ImageGenI32& lambda, int startX = 0, int startY = 0);
	AlignedImageF32 filter_generateF32(int width, int height, const ImageGenF32& lambda, int startX = 0, int startY = 0);

}

#endif

