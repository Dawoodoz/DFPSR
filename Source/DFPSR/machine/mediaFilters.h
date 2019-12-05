// zlib open source license
//
// Copyright (c) 2019 David Forsgren Piuva
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

#include "../../DFPSR/includeFramework.h" // TODO: Replace with specific modules

namespace dsr {

// Aliasing between a target and input image will increase reference count when input is given,
// detect target as shared and make a new allocation for the target.
// In other words, aliasing between input and output cannot be used to reduce the number of allocations.

void media_filter_add(AlignedImageU8& targetImage, AlignedImageU8 imageL, AlignedImageU8 imageR);
void media_filter_add(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint scalar);

void media_filter_sub(AlignedImageU8& targetImage, AlignedImageU8 imageL, AlignedImageU8 imageR);
void media_filter_sub(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint scalar);
void media_filter_sub(AlignedImageU8& targetImage, FixedPoint scalar, AlignedImageU8 image);

void media_filter_mul(AlignedImageU8& targetImage, AlignedImageU8 image, FixedPoint scalar);
void media_filter_mul(AlignedImageU8& targetImage, AlignedImageU8 imageL, AlignedImageU8 imageR, FixedPoint scalar);

// Fill a region of the image with a linear fade
void media_fade_region_linear(ImageU8& targetImage, const IRect& viewport, FixedPoint x1, FixedPoint y1, FixedPoint luma1, FixedPoint x2, FixedPoint y2, FixedPoint luma2);
// Fill the whole image with a linear fade
void media_fade_linear(ImageU8& targetImage, FixedPoint x1, FixedPoint y1, FixedPoint luma1, FixedPoint x2, FixedPoint y2, FixedPoint luma2);

// Fill a region of the image with a radial fade
// Pre-condition: innerRadius < outerRadius
//   outerRadius will silently be reassigned to innerRadius + epsilon if the criteria isn't met
void media_fade_region_radial(ImageU8& targetImage, const IRect& viewport, FixedPoint centerX, FixedPoint centerY, FixedPoint innerRadius, FixedPoint innerLuma, FixedPoint outerRadius, FixedPoint outerLuma);
// Fill the whole image with a radial fade
void media_fade_radial(ImageU8& targetImage, FixedPoint centerX, FixedPoint centerY, FixedPoint innerRadius, FixedPoint innerLuma, FixedPoint outerRadius, FixedPoint outerLuma);

}

#endif
