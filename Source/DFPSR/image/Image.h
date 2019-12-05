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

#ifndef DFPSR_IMAGE
#define DFPSR_IMAGE

#include <cassert>
#include <stdint.h>
#include "../base/SafePointer.h"
#include "../base/Buffer.h"
#include "../math/scalar.h"
#include "../math/IRect.h"
#include "PackOrder.h"

namespace dsr {

// See imageAPI.h for public methods
// See imageInternal.h for protected methods
class ImageImpl {
public:
	const int32_t width, height, stride, pixelSize;
	std::shared_ptr<Buffer> buffer; // Content
	const intptr_t startOffset; // Byte offset of the first pixel
	bool isSubImage = false;
private:
	void validate() {
		// Preconditions:
		assert(this->width > 0);
		assert(this->height > 0);
		assert(this->stride >= this->width * this->pixelSize);
		assert(this->pixelSize > 0);
		// TODO: Assert that the buffer is large enough to fit padding after each row
	}
public:
	// Sub-images
	ImageImpl(int32_t width, int32_t height, int32_t stride, int32_t pixelSize, std::shared_ptr<Buffer> buffer, intptr_t startOffset);
	// New images
	ImageImpl(int32_t width, int32_t height, int32_t stride, int32_t pixelSize);
};

#define IMAGE_DECLARATION(IMAGE_TYPE,CHANNELS,COLOR_TYPE,ELEMENT_TYPE) \
	static void writePixel(IMAGE_TYPE &image, int32_t x, int32_t y, COLOR_TYPE color); \
	static void writePixel_unsafe(IMAGE_TYPE &image, int32_t x, int32_t y, COLOR_TYPE color); \
	static COLOR_TYPE readPixel_clamp(const IMAGE_TYPE &image, int32_t x, int32_t y); \
	static COLOR_TYPE readPixel_unsafe(const IMAGE_TYPE &image, int32_t x, int32_t y);

}

#endif

