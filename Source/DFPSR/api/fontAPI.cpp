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

#include "fontAPI.h"
#include "imageAPI.h"
#include "../implementation/font/Font.h"
#include "../implementation/font/defaultFont.h"

namespace dsr {

static const RasterFont defaultFont = RasterFontImpl::createLatinOne(U"UbuntuMono", image_fromAscii(defaultFontAscii));

RasterFont font_getDefault() {
	return defaultFont;
}

RasterFont font_createLatinOne(const String& name, const ImageU8& atlas) {
	if (!image_exists(atlas)) {
		throwError(U"Cannot create the Latin-1 font called ", name, U" from an empty image handle.\n");
	} else if (image_getWidth(atlas) % 16 == 0 && image_getHeight(atlas) % 16 == 0
		&& image_getWidth(atlas) >= 16 && image_getHeight(atlas) >= 16) {
		throwError(U"Cannot create the Latin-1 font called ", name, U" from an image of ", image_getWidth(atlas), U"x", image_getHeight(atlas), U" pixels.\n");
	}
	return RasterFontImpl::createLatinOne(name, atlas);
}

String font_getName(const RasterFont font) {
	if (!font_exists(font)) {
		throwError(U"font_getName: font must exist!");
	}
	return font->name;
}

int32_t font_getSize(const RasterFont font) {
	if (!font_exists(font)) {
		throwError(U"font_getSize: font must exist!");
	}
	return font->size;
}

int32_t font_getSpacing(const RasterFont font) {
	if (!font_exists(font)) {
		throwError(U"font_getSpacing: font must exist!");
	}
	return font->spacing;
}

int32_t font_getTabWidth(const RasterFont font) {
	if (!font_exists(font)) {
		throwError(U"font_getTabWidth: font must exist!");
	}
	return font->tabWidth;
}

int32_t font_getCharacterWidth(const RasterFont font, DsrChar unicodeValue) {
	if (!font_exists(font)) {
		throwError(U"font_getCharacterWidth: font must exist!");
	}
	return font->getCharacterWidth(unicodeValue);
}

int32_t font_getMonospaceWidth(const RasterFont font) {
	if (!font_exists(font)) {
		throwError(U"font_getMonospaceWidth: font must exist!");
	}
	return font->widest + font->spacing;
}

int32_t font_getLineWidth(const RasterFont font, const ReadableString& content) {
	if (!font_exists(font)) {
		throwError(U"font_getLineWidth: font must exist!");
	}
	return font->getLineWidth(content);
}

int32_t font_printCharacter(ImageRgbaU8& target, const RasterFont font, DsrChar unicodeValue, const IVector2D& location, const ColorRgbaI32& color) {
	if (!image_exists(target)) {
		throwError(U"font_printCharacter: target must exist!");
	} else if (!font_exists(font)) {
		throwError(U"font_printCharacter: font must exist!");
	}
	return font->printCharacter(target, unicodeValue, location, color);
}

void font_printLine(ImageRgbaU8& target, const RasterFont font, const ReadableString& content, const IVector2D& location, const ColorRgbaI32& color) {
	if (!image_exists(target)) {
		throwError(U"font_printLine: target must exist!");
	} else if (!font_exists(font)) {
		throwError(U"font_printLine: font must exist!");
	} else {
		font->printLine(target, content, location, color);
	}
}

void font_printMultiLine(ImageRgbaU8& target, const RasterFont font, const ReadableString& content, const IRect& bound, const ColorRgbaI32& color) {
	if (!image_exists(target)) {
		throwError(U"font_printMultiLine: target must exist!");
	} else if (!font_exists(font)) {
		throwError(U"font_printMultiLine: font must exist!");
	} else {
		font->printMultiLine(target, content, bound, color);
	}
}

}

