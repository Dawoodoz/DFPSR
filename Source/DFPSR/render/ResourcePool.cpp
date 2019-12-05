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

#include "ResourcePool.h"
#include "../image/stbImage/stbImageWrapper.h"

using namespace dsr;

int BasicResourcePool::findImageRgba(const String& name) const {
	for (int i = 0; i < this->imageRgbaList.length(); i++) {
		// Warning!
		// This may cover up bugs with case sensitive matching in the Linux file system.
		// TODO: Make this case sensitive and enforce it on Windows or allow case insensitive loading on all systems.
		if (string_caseInsensitiveMatch(name, this->imageRgbaList[i].name)) {
			return i;
		}
	}
	return -1;
}

const ImageRgbaU8 BasicResourcePool::fetchImageRgba(const String& name) {
	ImageRgbaU8 result;
	// Using "" will return an empty reference to allow removing textures
	if (name.length() > 0) {
		int existingIndex = this->findImageRgba(name);
		if (existingIndex > -1) {
			result = imageRgbaList[existingIndex].ref;
		} else {
			// Look for a png image
			const String extensionless = this->path + name;
			result = image_load_RgbaU8(extensionless + ".png", false);
			// Look for gif
			if (!image_exists(result)) {
				result = image_load_RgbaU8(extensionless + ".gif", false);
			}
			// Look for jpg
			if (!image_exists(result)) {
				result = image_load_RgbaU8(extensionless + ".jpg", false);
			}
			if (image_exists(result)) {
				// If possible, generate a texture pyramid of smaller images
				if (image_isTexture(result)) {
					image_generatePyramid(result);
				}
				this->imageRgbaList.push(imageRgbaEntry(name, result));
			} else {
				printText("The image ", extensionless, ".* couldn't be loaded as either png, gif nor jpg!\n");
			}
		}
	}
	return result;
}
