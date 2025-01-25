
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

#ifndef DFPSR_API_DRAW
#define DFPSR_API_DRAW

#include "../image/Image.h"

namespace dsr {

// Instead of having lots of arguments for source and target regions, this library uses a system of sub-images to that any drawing method can be cropped.
//   To limit drawing to a rectangular target region:
//     * Create a sub-image using image_getSubImage.
//         0-----------------------------------X
//         | Parent-image                      |
//         |                                   |
//         |       -------------------         |
//         |      | IRect             |        |
//         |      |                   |        |
//         |      |                   |        |
//         |      |                   |        |
//         |       -------------------         |
//         |                                   |
//         Y-----------------------------------*
//     * Translate coordinates by subtracting the region's upper left corner.
//         0-----------------------------------X
//         | Parent-image                      |
//         |                                   |
//         |      0-------------------X        |
//         |      | Sub-image         |        |
//         |      |                   |        |
//         |      |                   |        |
//         |      |                   |        |
//         |      Y-------------------*        |
//         |                                   |
//         Y-----------------------------------*
//     * Draw to the new sub-image in the new local coordinate system.
//         0-------------------X
//         | Sub-image   /  |  |
//         |            /|  |__|
//         |   ________/ |  |  |
//         |          /  |     |
//         Y-------------------*

// Drawing shapes
	// TODO: Create wrappers taking left, top, width, height to reduce clutter from the IRect constructor.
	void draw_rectangle(const ImageU8& image, const IRect& bound, int color);
	void draw_rectangle(const ImageU16& image, const IRect& bound, int color);
	void draw_rectangle(const ImageF32& image, const IRect& bound, float color);
	void draw_rectangle(const ImageRgbaU8& image, const IRect& bound, const ColorRgbaI32& color);
	// Draw using a color that has been packed in advance with the same pack order using the image_saturateAndPack function.
	//   This saves time on saturation and packing when drawing many rectangles of the same color.
	void draw_rectangle(const ImageRgbaU8& image, const IRect& bound, uint32_t packedColor);

	// TODO: Also take two IVector2D as inlined wrapper functions.
	void draw_line(const ImageU8& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int color);
	void draw_line(const ImageU16& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int color);
	void draw_line(const ImageF32& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, float color);
	void draw_line(const ImageRgbaU8& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, const ColorRgbaI32& color);
	// Draw using a color that has been packed in advance with the same pack order using the image_saturateAndPack function.
	//   This saves time on saturation and packing when drawing many lines of the same color.
	void draw_line(const ImageRgbaU8& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t packedColor);

// Drawing images
	// Draw an image to another image
	//   All image types can draw to their own format
	//   All image types can draw to RgbaU8
	//   All monochrome types can draw to each other
	//   The source and target images can be sub-images from the same atlas but only if the sub-regions are not overlapping
	void draw_copy(const ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageU8& target, const ImageU8& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageU16& target, const ImageU16& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageF32& target, const ImageF32& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageRgbaU8& target, const ImageU8& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageRgbaU8& target, const ImageU16& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageRgbaU8& target, const ImageF32& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageU8& target, const ImageF32& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageU8& target, const ImageU16& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageU16& target, const ImageU8& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageU16& target, const ImageF32& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageF32& target, const ImageU8& source, int32_t left = 0, int32_t top = 0);
	void draw_copy(const ImageF32& target, const ImageU16& source, int32_t left = 0, int32_t top = 0);
	// Draw one RGBA image to another using alpha filtering
	//   Target alpha does no affect RGB blending, in case that it contains padding for opaque targets
	//   If you really want to draw to a transparent layer, this method should not be used
	void draw_alphaFilter(const ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left = 0, int32_t top = 0);
	// Draw one RGBA image to another using the alpha channel as height
	//   sourceAlphaOffset is added to non-zero heights from source alpha
	//   Writes each source pixel who's alpha value is greater than the target's
	//   Zero alpha can be used as a mask, because no source value can be below zero in unsigned color formats
	void draw_maxAlpha(const ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left = 0, int32_t top = 0, int32_t sourceAlphaOffset = 0);

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
	void draw_higher(const ImageU16& targetHeight, const ImageU16& sourceHeight,
		ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
		int32_t left = 0, int32_t top = 0, int32_t sourceHeightOffset = 0
	);
	void draw_higher(const ImageU16& targetHeight, const ImageU16& sourceHeight,
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
	void draw_higher(const ImageF32& targetHeight, const ImageF32& sourceHeight,
		ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
		int32_t left = 0, int32_t top = 0, float sourceHeightOffset = 0
	);
	void draw_higher(const ImageF32& targetHeight, const ImageF32& sourceHeight,
		ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
		ImageRgbaU8& targetB, const ImageRgbaU8& sourceB,
		int32_t left = 0, int32_t top = 0, float sourceHeightOffset = 0
	);

	// TODO: Inlined wrappers using IVector2D.
	// Draw one RGBA image to another using alpha clipping
	//   Source is solid where alpha is greater than threshold, which can be used for animations
	void draw_alphaClip(const ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left = 0, int32_t top = 0, int32_t threshold = 127);
	// Draw a uniform color using a grayscale silhouette as the alpha channel
	void draw_silhouette(const ImageRgbaU8& target, const ImageU8& silhouette, const ColorRgbaI32& color, int32_t left = 0, int32_t top = 0);

}

#endif

