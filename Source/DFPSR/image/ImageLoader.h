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

#ifndef DFPSR_IMAGE_LOADER
#define DFPSR_IMAGE_LOADER

#include "ImageRgbaU8.h"
#include "../base/text.h"
#include <stdio.h>

namespace dsr {

// When you want to load an image and be able to edit the content,
// the image loader can be called directly instead of using the
// resource pool where everything has to be write-protected for reuse.
class ImageLoader {
public:
	// Load an image from a file. PNG support is a minimum requirement.
	virtual ImageRgbaU8Impl loadAsRgba(const String& filename) const = 0;
	// Save an image in the PNG format with the given filename.
	// Returns true on success and false on failure.
	virtual bool saveAsPng(const ImageRgbaU8Impl &image, const String& filename) const {
		printText("saveAsPng is not yet implemented in the image loader!");
		return false;
	}
};

}

#endif

