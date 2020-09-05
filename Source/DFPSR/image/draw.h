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

#ifndef DFPSR_IMAGE_DRAW
#define DFPSR_IMAGE_DRAW

#include "Image.h"
#include "ImageU8.h"
#include "ImageU16.h"
#include "ImageF32.h"
#include "ImageRgbaU8.h"

namespace dsr {

// An internal draw API to allow having multiple external APIs without code duplication

void imageImpl_draw_solidRectangle(ImageU8Impl& image, const IRect& bound, int color);
void imageImpl_draw_solidRectangle(ImageU16Impl& image, const IRect& bound, int color);
void imageImpl_draw_solidRectangle(ImageF32Impl& image, const IRect& bound, float color);
void imageImpl_draw_solidRectangle(ImageRgbaU8Impl& image, const IRect& bound, const ColorRgbaI32& color);

void imageImpl_draw_line(ImageU8Impl& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int color);
void imageImpl_draw_line(ImageU16Impl& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int color);
void imageImpl_draw_line(ImageF32Impl& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, float color);
void imageImpl_draw_line(ImageRgbaU8Impl& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, const ColorRgbaI32& color);

// Integer formats of different size are treated as having the same scale but different ranges
void imageImpl_drawCopy(ImageRgbaU8Impl& target, const ImageRgbaU8Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageU8Impl& target, const ImageU8Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageU16Impl& target, const ImageU16Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageF32Impl& target, const ImageF32Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageRgbaU8Impl& target, const ImageU8Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageRgbaU8Impl& target, const ImageU16Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageRgbaU8Impl& target, const ImageF32Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageU8Impl& target, const ImageF32Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageU8Impl& target, const ImageU16Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageU16Impl& target, const ImageU8Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageU16Impl& target, const ImageF32Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageF32Impl& target, const ImageU8Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawCopy(ImageF32Impl& target, const ImageU16Impl& source, int32_t left = 0, int32_t top = 0);

void imageImpl_drawAlphaFilter(ImageRgbaU8Impl& target, const ImageRgbaU8Impl& source, int32_t left = 0, int32_t top = 0);
void imageImpl_drawMaxAlpha(ImageRgbaU8Impl& target, const ImageRgbaU8Impl& source, int32_t left = 0, int32_t top = 0, int32_t sourceAlphaOffset = 0);
void imageImpl_drawAlphaClip(ImageRgbaU8Impl& target, const ImageRgbaU8Impl& source, int32_t left = 0, int32_t top = 0, int32_t threshold = 0);
void imageImpl_drawSilhouette(ImageRgbaU8Impl& target, const ImageU8Impl& source, const ColorRgbaI32& color, int32_t left = 0, int32_t top = 0);

void imageImpl_drawHigher(ImageU16Impl& targetHeight, const ImageU16Impl& sourceHeight, int32_t left = 0, int32_t top = 0, int32_t sourceHeightOffset = 0);
void imageImpl_drawHigher(ImageU16Impl& targetHeight, const ImageU16Impl& sourceHeight, ImageRgbaU8Impl& targetA, const ImageRgbaU8Impl& sourceA,
  int32_t left = 0, int32_t top = 0, int32_t sourceHeightOffset = 0);
void imageImpl_drawHigher(ImageU16Impl& targetHeight, const ImageU16Impl& sourceHeight, ImageRgbaU8Impl& targetA, const ImageRgbaU8Impl& sourceA,
  ImageRgbaU8Impl& targetB, const ImageRgbaU8Impl& sourceB, int32_t left = 0, int32_t top = 0, int32_t sourceHeightOffset = 0);
void imageImpl_drawHigher(ImageF32Impl& targetHeight, const ImageF32Impl& sourceHeight, int32_t left = 0, int32_t top = 0, float sourceHeightOffset = 0);
void imageImpl_drawHigher(ImageF32Impl& targetHeight, const ImageF32Impl& sourceHeight, ImageRgbaU8Impl& targetA, const ImageRgbaU8Impl& sourceA,
  int32_t left = 0, int32_t top = 0, float sourceHeightOffset = 0);
void imageImpl_drawHigher(ImageF32Impl& targetHeight, const ImageF32Impl& sourceHeight, ImageRgbaU8Impl& targetA, const ImageRgbaU8Impl& sourceA,
  ImageRgbaU8Impl& targetB, const ImageRgbaU8Impl& sourceB, int32_t left = 0, int32_t top = 0, float sourceHeightOffset = 0);

// Pre-conditions:
//     * wideTempImage should be one of the following:
//        * A nullptr (for allocating it automatically when needed)
//          Can be preferred when down-scaling, because the two-step resize is only used when width changes and height increases
//        * An image of dimensions target.width x source.height and the same pack order as target
//          Wrong dimensions or pack order for wideTempImage is equivalent to passing nullptr
//     * target must own its padding
//       This is automatically true for aligned images
//       If broken, visible pixels in a parent image may change outside of the sub-image's region
// Side-effects:
//     * Writes a resized version of source to target, including padding
//     * May also write to any pixels in wideTempImage, including padding
//     * May also change the pack order of wideTempImage
void imageImpl_resizeInPlace(ImageRgbaU8Impl& target, ImageRgbaU8Impl* wideTempImage, const ImageRgbaU8Impl& source, bool interpolate, const IRect& scaleRegion);
void imageImpl_resizeToTarget(ImageRgbaU8Impl& target, const ImageRgbaU8Impl& source, bool interpolate);
void imageImpl_blockMagnify(ImageRgbaU8Impl& target, const ImageRgbaU8Impl& source, int pixelWidth, int pixelHeight);
void imageImpl_blockMagnify_aligned(ImageRgbaU8Impl& target, const ImageRgbaU8Impl& source, int pixelWidth, int pixelHeight);

}

#endif

