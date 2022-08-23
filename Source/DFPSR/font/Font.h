// zlib open source license
//
// Copyright (c) 2018 to 2022 David Forsgren Piuva
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

#ifndef DFPSR_GUI_FONT
#define DFPSR_GUI_FONT

#include "../api/types.h"
#include "../collection/List.h"

namespace dsr {

struct RasterCharacter {
public:
	// Image to draw
	ImageU8 image;
	// Look-up value
	DsrChar unicodeValue = 0;
	// The width of the character
	int32_t width = 0;
	// Y offset
	int32_t offsetY = 0;
public:
	// Constructor
	RasterCharacter() {}
	RasterCharacter(const ImageU8& image, DsrChar unicodeValue, int32_t offsetY);
	// Destructor
	~RasterCharacter() {}
};

class RasterFontImpl {
public:
	// Font identity
	const String name;
	const int32_t size = 0; // From the top of one row to another
	// Settings
	int32_t spacing = 0; // The extra pixels between each character
	int32_t spaceWidth = 0; // The size of a whole space character including spacing
	int32_t tabWidth = 0; // The size of a whole tab including spacing
	int32_t widest = 0; // The maximum character width excluding spacing
	// A list of character images with their unicode keys
	List<RasterCharacter> characters;
	// TODO: A way to map all UTF-32 characters
	// Indices to characters within the 16-bit range
	//  indices[x] = -1 for non-existing character codes
	//  The indices[0..255] contains the Latin-1 subset
	int32_t indices[65536];
public:
	// Constructor
	RasterFontImpl(const String& name, int32_t size, int32_t spacing, int32_t spaceWidth);
	static std::shared_ptr<RasterFontImpl> createLatinOne(const String& name, const ImageU8& atlas);
	// Destructor
	~RasterFontImpl();
public:
	// Allready registered unicode characters will be ignored if reused, so load overlapping sets in order of priority
	void registerCharacter(const ImageU8& image, DsrChar unicodeValue, int32_t offsetY);
	// Call after construction to register up to 256 characters in a 16x16 grid from the atlas
	void registerLatinOne16x16(const ImageU8& atlas);
	// Returns the width of a character including spacing in pixels
	int32_t getCharacterWidth(DsrChar unicodeValue) const;
	// Returns the total length of characters in pixels as if printing content
	// If multiple lines exists it will simply keep adding to the total by ignoring line-breaks
	int64_t getLineWidth(const ReadableString& content) const;
	// Prints a character and returns the horizontal stride in pixels
	int32_t printCharacter(ImageRgbaU8& target, DsrChar unicodeValue, const IVector2D& location, const ColorRgbaI32& color) const;
	// Prints a whole line of text from location
	void printLine(ImageRgbaU8& target, const ReadableString& content, const IVector2D& location, const ColorRgbaI32& color) const;
	// Prints multiple lines of text within a bound
	void printMultiLine(ImageRgbaU8& target, const ReadableString& content, const IRect& bound, const ColorRgbaI32& color) const;
};

// See DFPSR/api/fontAPI.h for the procedural interface

}

#endif

