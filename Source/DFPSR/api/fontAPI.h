// zlib open source license
//
// Copyright (c) 2020 to 2022 David Forsgren Piuva
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

#ifndef DFPSR_API_FONT
#define DFPSR_API_FONT

#include "../implementation/image/Image.h"
#include "stringAPI.h"

namespace dsr {
	// A handle to a raster font
	class RasterFontImpl;
	using RasterFont = Handle<RasterFontImpl>;

	// Get a handle to the default font
	RasterFont font_getDefault();
	// Create a new font mapped to the Latin-1 character sub-set using a fixed size grid of 16 x 16 sub-images
	//   atlas contains 16 x 16 character images starting with character codes 0 to 15 and continuing left to right on the next cell rows in the atlas
	// Pre-conditions:
	//   * atlas must exist
	//   * atlas must have dimensions evenly divisible by 16
	//     image_getWidth(atlas) % 16 == 0
	//     image_getHeight(atlas) % 16 == 0
	//   * Each cell must include at least one pixel
	//     image_getWidth(atlas) >= 16
	//     image_getHeight(atlas) >= 16
	RasterFont font_createLatinOne(const String& name, const ImageU8& atlas);
	// Post-condition: Returns true iff font exists
	inline bool font_exists(const RasterFont font) { return font.isNotNull(); }
	// Pre-condition: font must exist
	// Post-condition: Returns font's name, as given on construction
	String font_getName(const RasterFont font);
	// Pre-condition: font must exist
	// Post-condition:
	//   Returns the font's size in pixels, which is the height of an individual character image before cropping
	//   If ceated using font_createLatinOne then it's image_getHeight(atlas) / 16
	int32_t font_getSize(const RasterFont font);
	// Pre-condition: font must exist
	// Post-condition: Returns the font's empty space between characters in pixels.
	int32_t font_getSpacing(const RasterFont font);
	// Pre-condition: font must exist
	// Post-condition: Returns the font's maximum tab width in pixels, which is the alignment the write location will jump to when reading a tab.
	int32_t font_getTabWidth(const RasterFont font);
	// Pre-condition: font must exist
	// Post-condition: Returns the width of a character including spacing in pixels
	int32_t font_getCharacterWidth(const RasterFont font, DsrChar unicodeValue);
	// Pre-condition: font must exist
	// Post-condition: Returns the width of the widest character including spacing in pixels
	int32_t font_getMonospaceWidth(const RasterFont font);
	// Pre-condition: font must exist
	// Post-condition: Returns the total length of content in pixels while ignoring line-breaks
	int32_t font_getLineWidth(const RasterFont font, const ReadableString& content);
	// Pre-condition: font and target must exist
	// Side-effects: Prints a character and returns the horizontal stride in pixels
	int32_t font_printCharacter(ImageRgbaU8& target, const RasterFont font, DsrChar unicodeValue, const IVector2D& location, const ColorRgbaI32& color);
	// Pre-condition: font and target must exist
	// Side-effects: Prints content from location while ignoring line-breaks
	void font_printLine(ImageRgbaU8& target, const RasterFont font, const ReadableString& content, const IVector2D& location, const ColorRgbaI32& color);
	// Pre-condition: font and target must exist
	// Side-effects: Prints multiple lines of text within bound
	// Guarantees that:
	//   * No characters are clipped against bound
	//     It may however clip against the target image's bound if you make the bound larger than the image for partial updates or scrolling effects
	//   * No pixels are drawn outside of bound
	void font_printMultiLine(ImageRgbaU8& target, const RasterFont font, const ReadableString& content, const IRect& bound, const ColorRgbaI32& color);
}

#endif

