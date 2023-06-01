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

#ifndef DFPSR_IMAGE_U8
#define DFPSR_IMAGE_U8

#include "Image.h"

namespace dsr {

class ImageU8Impl : public ImageImpl {
public:
	static const int32_t channelCount = 1;
	static const int32_t pixelSize = channelCount;
	// Inherit constructors
	using ImageImpl::ImageImpl;
	// Constructors
	ImageU8Impl(int32_t newWidth, int32_t newHeight, int32_t newStride, Buffer buffer, intptr_t startOffset);
	ImageU8Impl(int32_t newWidth, int32_t newHeight, int32_t alignment);
	// Macro defined functions
	IMAGE_DECLARATION(ImageU8Impl, 1, uint8_t, uint8_t);
};

}

#endif
