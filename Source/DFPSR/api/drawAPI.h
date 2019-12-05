
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

#ifndef DFPSR_API_DRAW
#define DFPSR_API_DRAW

#include "types.h"
#include <functional>

namespace dsr {



// ------------------------ Below is untested! ------------------------ //



// Drawing shapes
	void draw_rectangle(ImageU8& image, const IRect& bound, int color);
	void draw_rectangle(ImageF32& image, const IRect& bound, float color);
	void draw_rectangle(ImageRgbaU8& image, const IRect& bound, const ColorRgbaI32& color);

	void draw_line(ImageU8& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int color);
	void draw_line(ImageF32& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, float color);
	void draw_line(ImageRgbaU8& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, const ColorRgbaI32& color);

// Drawing images
	// Draw an image to another image
	//   All image types can draw to their own format
	//   All image types can draw to RgbaU8
	//   All monochrome types can draw to each other
	//   The source and target images can be sub-images from the same atlas but only if the sub-regions are not overlapping
	void draw_copy(ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageU8& target, const ImageU8& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageU16& target, const ImageU16& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageF32& target, const ImageF32& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageRgbaU8& target, const ImageU8& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageRgbaU8& target, const ImageU16& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageRgbaU8& target, const ImageF32& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageU8& target, const ImageF32& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageU8& target, const ImageU16& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageU16& target, const ImageU8& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageU16& target, const ImageF32& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageF32& target, const ImageU8& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(ImageF32& target, const ImageU16& source, int32_t left = 0, int32_t top = 0);
	// Draw one RGBA image to another using alpha filtering
	//   Target alpha does no affect RGB blending, in case that it contains padding for opaque targets
	//   If you really want to draw to a transparent layer, this method should not be used
	void draw_alphaFilter(ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left = 0, int32_t top = 0);
	// Draw one RGBA image to another using the alpha channel as height
	//   sourceAlphaOffset is added to non-zero heights from source alpha
	//   Writes each source pixel who's alpha value is greater than the target's
	//   Zero alpha can be used as a mask, because no source value can be below zero in unsigned color formats
	void draw_maxAlpha(ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left = 0, int32_t top = 0, int32_t sourceAlphaOffset = 0);

	// Draw between multiple images using a height buffer
	//   Each source pixel is drawn where the source height's pixel exceeds the target height's pixel
	//   Including the source height pixel, so that the drawn object occludes the following objects below it
	//   Can be used for isometric top-down and side-scroller games with heavy graphical effects
	//   A usually contains color pixels
	//   B usually contains surface normals for light effects
	// 16-bit integer depth buffers:
	//   Fully deterministic overlaps
	//   Source height zero is treated as invisible even if sourceHeightOffset adds to the height
	//   It's recommended to let the target height buffer use 32768 as height zero to allow placing things on negative locations
	void draw_higher(
		ImageU16& targetHeight, const ImageU16& sourceHeight,
		int32_t left = 0, int32_t top = 0, int32_t sourceHeightOffset = 0
	);
	void draw_higher(ImageU16& targetHeight, const ImageU16& sourceHeight,
		ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
		int32_t left = 0, int32_t top = 0, int32_t sourceHeightOffset = 0
	);
	void draw_higher(ImageU16& targetHeight, const ImageU16& sourceHeight,
		ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
		ImageRgbaU8& targetB, const ImageRgbaU8& sourceB,
		int32_t left = 0, int32_t top = 0, int32_t sourceHeightOffset = 0
	);
	// 32-bit floating-point depth buffers
	//   Source height negative infinity is used for invisible pixels
	//     Negative infinity is expressed using -std::numeric_limits<float>::infinity() from limits.h
	//   Same pixel size as in ImageRgbaU8 to make aligned reading easier when used together with colors
	//   Floats allow doing light calculations directly without having to perform expensive conversions from integers
	void draw_higher(
		ImageF32& targetHeight, const ImageF32& sourceHeight,
		int32_t left = 0, int32_t top = 0, float sourceHeightOffset = 0
	);
	void draw_higher(ImageF32& targetHeight, const ImageF32& sourceHeight,
		ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
		int32_t left = 0, int32_t top = 0, float sourceHeightOffset = 0
	);
	void draw_higher(ImageF32& targetHeight, const ImageF32& sourceHeight,
		ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
		ImageRgbaU8& targetB, const ImageRgbaU8& sourceB,
		int32_t left = 0, int32_t top = 0, float sourceHeightOffset = 0
	);

	// Draw one RGBA image to another using alpha clipping
	//   Source is solid where alpha is greater than treshold, which can be used for animations
	void draw_alphaClip(ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left = 0, int32_t top = 0, int32_t treshold = 127);
	// Draw a uniform color using a grayscale silhouette as the alpha channel
	void draw_silhouette(ImageRgbaU8& target, const ImageU8& silhouette, const ColorRgbaI32& color, int32_t left = 0, int32_t top = 0);

// TODO: Make a separate filter API

// Image resizing
	// The interpolate argument
	//   Bi-linear interoplation is used when true
	//   Nearest neighbor sampling is used when false
	// Create a stretched version of the source image with the given dimensions and default RGBA pack order
	OrderedImageRgbaU8 filter_resize(const ImageRgbaU8& image, Sampler interpolation, int32_t newWidth, int32_t newHeight);
	// Resize with borders of pixels that aren't stretched
	//   Using a larger border than half the size will be clamped, so that the center keeps at least 2x2 pixels
	OrderedImageRgbaU8 filter_resize3x3(const ImageRgbaU8& image, Sampler interpolation, int newWidth, int newHeight, int leftBorder, int topBorder, int rightBorder, int bottomBorder);
	// The source image is scaled by pixelWidth and pixelHeight from the upper left corner
	// If source is too small, transparent black pixels (0, 0, 0, 0) fills the outside
	// If source is too large, partial pixels will be cropped away completely and replaced by the black border
	// Letting the images have the same pack order and be aligned to 16-bytes will increase speed
	void filter_blockMagnify(ImageRgbaU8& target, const ImageRgbaU8& source, int pixelWidth, int pixelHeight);

// Image generation and filtering
//   Create new images from Lambda expressions
//   Useful for pre-generating images for reuse, reference implementations and fast prototyping
	// Lambda expressions for generating integer images
	using ImageGenRgbaU8 = std::function<ColorRgbaI32(int, int)>;
	using ImageGenF32 = std::function<float(int, int)>;
	// In-place image generation to an existing image
	//   The pixel at the upper left corner gets (startX, startY) as x and y arguments to the function
	void filter_mapRgbaU8(ImageRgbaU8& target, const ImageGenRgbaU8& lambda, int startX = 0, int startY = 0);
	void filter_mapF32(ImageF32& target, const ImageGenF32& lambda, int startX = 0, int startY = 0);
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
	ImageRgbaU8 filter_generateRgbaU8(int width, int height, const ImageGenRgbaU8& lambda, int startX = 0, int startY = 0);
	ImageF32 filter_generateF32(int width, int height, const ImageGenF32& lambda, int startX = 0, int startY = 0);

	// TODO: Document
	ImageRgbaU8 filter_mulColorRgb(const ImageRgbaU8& inputImage, const ColorRgbI32& color);
	ImageRgbaU8 filter_mulColorRgba(const ImageRgbaU8& inputImage, const ColorRgbaI32& color);
}

#endif

