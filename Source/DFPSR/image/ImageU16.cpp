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

#include "ImageU16.h"
#include "internal/imageInternal.h"
#include "internal/imageTemplate.h"

using namespace dsr;

ImageU16Impl::ImageU16Impl(int32_t newWidth, int32_t newHeight, int32_t newStride, Buffer buffer, intptr_t startOffset) :
  ImageImpl(newWidth, newHeight, newStride, sizeof(uint16_t), buffer, startOffset) {
	assert(buffer_getSize(buffer) - startOffset >= imageInternal::getUsedBytes(this));
}

ImageU16Impl::ImageU16Impl(int32_t newWidth, int32_t newHeight, int32_t alignment) :
  ImageImpl(newWidth, newHeight, roundUp(newWidth * sizeof(uint16_t), alignment), sizeof(uint16_t)) {
}

IMAGE_DEFINITION(ImageU16Impl, 1, uint16_t, uint16_t);
