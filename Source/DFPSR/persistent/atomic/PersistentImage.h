// zlib open source license
//
// Copyright (c) 2021 David Forsgren Piuva
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

#ifndef DFPSR_GUI_PERSISTENTIMAGE
#define DFPSR_GUI_PERSISTENTIMAGE

#include "../ClassFactory.h"
#include "../../api/imageAPI.h"

namespace dsr {

// A container for images that can either embed the image as hexadecimals or save a reference a the file's path, depending on if the path is set.
class PersistentImage : public Persistent {
public:
	PERSISTENT_DECLARATION(PersistentImage)
	OrderedImageRgbaU8 value; // Persistent images may not have native pack order, because it would cause incompatibility.
	String path; // If the path has any characters, serialization will save the image's file path instead of the pixel content to save space.
public:
	PersistentImage() {}
public:
	virtual bool assignValue(const ReadableString &text) override;
	virtual String& toStreamIndented(String& out, const ReadableString& indentation) const override;
};

}

#endif
