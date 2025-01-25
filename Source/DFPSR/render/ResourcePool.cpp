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

#include "ResourcePool.h"
#include "../image/stbImage/stbImageWrapper.h"
#include "../api/fileAPI.h"
#include "../api/imageAPI.h"
#include "../api/textureAPI.h"

using namespace dsr;

int BasicResourcePool::findImageRgba(const ReadableString& name) const {
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

int BasicResourcePool::findTextureRgba(const ReadableString& name) const {
	for (int i = 0; i < this->textureRgbaList.length(); i++) {
		if (string_caseInsensitiveMatch(name, this->textureRgbaList[i].name)) {
			return i;
		}
	}
	return -1;
}

const ImageRgbaU8 BasicResourcePool::fetchImageRgba(const ReadableString& name) {
	ImageRgbaU8 result;
	// Using "" will return an empty reference to allow removing textures
	if (string_length(name) > 0) {
		int existingIndex = this->findImageRgba(name);
		if (existingIndex > -1) {
			result = this->imageRgbaList[existingIndex].resource;
		} else if (string_findFirst(name, U'.') > -1) {
			throwError("The image \"", name, "\" had a forbidden dot in the name. Images in resource pools are fetched without the extension to allow changing image format without changing what it's called in other resources.\n");
		} else if (string_findFirst(name, U'/') > -1 && string_findFirst(name, U'\\') > -1) {
			throwError("The image \"", name, "\" contained a path separator, which is not allowed because of ambiguity. The same file can have multiple paths to the same folder and multiple files can have the same name in different folders.\n");
		} else {
			// Look for a png image
			const String extensionless = file_combinePaths(this->path, name);
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
				this->imageRgbaList.push(namedEntry<ImageRgbaU8>(name, result));
			} else {
				printText("The image ", extensionless, ".* couldn't be loaded as either png, gif nor jpg!\n");
			}
		}
	}
	return result;
}

const TextureRgbaU8 BasicResourcePool::fetchTextureRgba(const ReadableString& name, int32_t resolutions) {
	TextureRgbaU8 result;
	// Using "" will return an empty reference to allow removing textures
	if (string_length(name) > 0) {
		int existingTextureIndex = this->findTextureRgba(name);
		int existingImageIndex = this->findImageRgba(name);
		if (existingTextureIndex > -1) {
			result = this->textureRgbaList[existingTextureIndex].resource;
		} else if (existingImageIndex > -1) {
			result = texture_create_RgbaU8(this->imageRgbaList[existingImageIndex].resource, resolutions);
		} else {
			// TODO: Save memory by loading a temporary image for generating the texture
			//         and letting the image point to the highest layer in the texture using texture_getMipLevelImage(result, 0).
			result = texture_create_RgbaU8(this->fetchImageRgba(name), resolutions);
			/* Enable this to save each texture layer as a file for debugging.
			if (texture_exists(result)) {
				for (int32_t mipLevel = 0; mipLevel < texture_getSmallestMipLevel(result); mipLevel++) {
					image_save(texture_getMipLevelImage(result, mipLevel), string_combine(U"Mip_", name, U"_", texture_getWidth(result, mipLevel), U"x", texture_getHeight(result, mipLevel), U".png"));
				}
			}
			*/
		}
		if (texture_exists(result)) {
			this->textureRgbaList.push(namedEntry<TextureRgbaU8>(name, result));
		}
	}
	return result;
}
