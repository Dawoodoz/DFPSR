// zlib open source license
//
// Copyright (c) 2019 to 2022 David Forsgren Piuva
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

#ifndef DFPSR_MEDIA_FILTERS
#define DFPSR_MEDIA_FILTERS

#include "../../api/imageAPI.h"
#include "../../math/FixedPoint.h"

namespace dsr {

// No float nor double allowed in Media Filters.
//   Every bit has to be 100% deterministic across different computers if they use the same version of the library.
//   How a specific version works may however change how rounding is done in order to improve speed and precision.

void media_filter_add(AlignedImageU8& targetImage, AlignedImageU8 imageA, AlignedImageU8 imageB);
void media_filter_add(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint luma);
void media_filter_add(AlignedImageU8& targetImage, AlignedImageU8 image, int32_t luma);

void media_filter_sub(AlignedImageU8& targetImage, AlignedImageU8 imageA, AlignedImageU8 imageB);
void media_filter_sub(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint luma);
void media_filter_sub(AlignedImageU8& targetImage, FixedPoint luma, AlignedImageU8 image);
void media_filter_sub(AlignedImageU8& targetImage, AlignedImageU8 image, int32_t luma);
void media_filter_sub(AlignedImageU8& targetImage, int32_t luma, AlignedImageU8 image);

void media_filter_mul(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint scalar);
void media_filter_mul(AlignedImageU8& targetImage, AlignedImageU8 imageA, AlignedImageU8 imageB, FixedPoint scalar);

// Fill a region of the image with a linear fade, so that the pixel at (x1, y1) becomes roughly luma1, and the pixel at (x2, y2) becomes roughly luma2.
// Fills entirely with luma1 if x1 == x2 and y1 == y2 (the line has no direction).
// Safely crops the viewport to targetImage if too big.
void media_fade_region_linear(ImageU8& targetImage, const IRect& viewport, FixedPoint x1, FixedPoint y1, FixedPoint luma1, FixedPoint x2, FixedPoint y2, FixedPoint luma2);
// Fill the whole image with a linear fade.
void media_fade_linear(ImageU8& targetImage, FixedPoint x1, FixedPoint y1, FixedPoint luma1, FixedPoint x2, FixedPoint y2, FixedPoint luma2);

// Fill a region of the image with a radial fade.
// Safely crops the viewport to targetImage if too big.
// Pre-condition: innerRadius < outerRadius.
//   outerRadius will silently be reassigned to innerRadius + epsilon if the criteria isn't met.
void media_fade_region_radial(ImageU8& targetImage, const IRect& viewport, FixedPoint centerX, FixedPoint centerY, FixedPoint innerRadius, FixedPoint innerLuma, FixedPoint outerRadius, FixedPoint outerLuma);
// Fill the whole image with a radial fade.
void media_fade_radial(ImageU8& targetImage, FixedPoint centerX, FixedPoint centerY, FixedPoint innerRadius, FixedPoint innerLuma, FixedPoint outerRadius, FixedPoint outerLuma);

}

#endif
