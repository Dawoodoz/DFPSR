﻿// zlib open source license
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

#ifndef DFPSR_RENDER_RESOURCE_POOL
#define DFPSR_RENDER_RESOURCE_POOL

#include "../image/Texture.h"
#include "../../collection/List.h"
#include "../../api/stringAPI.h"

namespace dsr {

// A resource pool is responsible for storing things that might be reused in order to avoid loading the same file multiple times
class ResourcePool {
public:
	virtual const ImageRgbaU8 fetchImageRgba(const ReadableString& name) = 0;
	virtual const TextureRgbaU8 fetchTextureRgba(const ReadableString& name, int32_t resolutions = 4) = 0;
};

// TODO: Keep track of reference count to resources and have a clean-up method for removing unused resources.
template <typename T>
struct namedEntry {
	String name;
	const T resource;
	namedEntry(const String& name, const T& resource) : name(name), resource(resource) {}
};

class BasicResourcePool : public ResourcePool {
private:
	List<namedEntry<ImageRgbaU8>> imageRgbaList;
	List<namedEntry<TextureRgbaU8>> textureRgbaList;
	int findImageRgba(const ReadableString& name) const;
	int findTextureRgba(const ReadableString& name) const;
public:
	String path;
	explicit BasicResourcePool(const ReadableString& path) : path(path) {}
	const ImageRgbaU8 fetchImageRgba(const ReadableString& name) override;
	// The resolutions argument can be used to limit the number of mip levels for a specific rendering engine.
	const TextureRgbaU8 fetchTextureRgba(const ReadableString& name, int32_t resolutions) override;
};

}

#endif

