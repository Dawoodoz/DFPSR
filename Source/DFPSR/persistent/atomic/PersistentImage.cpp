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

#include "PersistentImage.h"
#include "../../api/fileAPI.h"

using namespace dsr;

PERSISTENT_DEFINITION(PersistentImage)

static uint8_t readHexaDecimal(const ReadableString &text, int &readFrom) {
	uint8_t result = 0u;
	for (int i = 0; i < 2; i++) {
		result = result << 4;
		DsrChar c = text[readFrom];
		if (U'0' <= c && c <= U'9') {
			result = result | (c - U'0');
		} else if (U'a' <= c && c <= U'f') {
			result = result | (c - U'a' + 10);
		} else if (U'A' <= c && c <= U'F') {
			result = result | (c - U'A' + 10);
		}
		readFrom++;
	}
	return result;
}

bool PersistentImage::assignValue(const ReadableString &text, const ReadableString &fromPath) {
	if (string_caseInsensitiveMatch(text, U"NONE")) {
		// Set the handle to null
		this->value = OrderedImageRgbaU8();
	} else {
		// Create an image from the text
		int colonIndex = string_findFirst(text, U':');
		if (colonIndex == -1) {
			printText("Missing colon when creating PersistentImage from text!\n");
			return false;
		}
		ReadableString leftSide = string_before(text, colonIndex);
		if (string_caseInsensitiveMatch(leftSide, U"FILE")) {
			// Read image from the file path
			String absolutePath = file_getTheoreticalAbsolutePath(string_after(text, colonIndex), fromPath);
			this->value = image_load_RgbaU8(absolutePath);
		} else {
			// Read dimensions and a sequence of pixels as hexadecimals
			int xIndex = string_findFirst(text, U'x');
			if (xIndex == -1 || xIndex > colonIndex) {
				printText("Missing x when parsing embedded PersistentImage from text!\n");
				return false;
			}
			int width = string_toInteger(string_before(leftSide, xIndex));
			int height = string_toInteger(string_after(leftSide, xIndex));
			if (width <= 0 || height <= 0) {
				// No pixels found
				this->value = OrderedImageRgbaU8();
			} else {
				this->value = image_create_RgbaU8(width, height);
				int readIndex = colonIndex + 1;
				for (int y = 0; y < height; y++) {
					for (int x = 0; x < width; x++) {
						int red = readHexaDecimal(text, readIndex);
						int green = readHexaDecimal(text, readIndex);
						int blue = readHexaDecimal(text, readIndex);
						int alpha = readHexaDecimal(text, readIndex);
						image_writePixel(this->value, x, y, ColorRgbaI32(red, green, blue, alpha));
					}
				}
			}
		}
	}
	return true;
}

static const String hexadecimals = U"0123456789ABCDEF";
static void writeHexaDecimal(String &out, uint8_t value) {
	string_appendChar(out, hexadecimals[(value & 0b11110000) >> 4]);
	string_appendChar(out, hexadecimals[value & 0b00001111]);
}
String& PersistentImage::toStreamIndented(String &out, const ReadableString &indentation) const {
	string_append(out, indentation);
	if (string_length(this->path)) {
		string_append(out, "File:", this->path);
	} else if (image_exists(this->value)) {
		int width = image_getWidth(this->value);
		int height = image_getHeight(this->value);
		string_append(out, width, U"x", height, U":");
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				ColorRgbaI32 color = image_readPixel_clamp(this->value, x, y);
				writeHexaDecimal(out, color.red);
				writeHexaDecimal(out, color.green);
				writeHexaDecimal(out, color.blue);
				writeHexaDecimal(out, color.alpha);
			}
		}
	} else {
		string_append(out, U"None");
	}
	return out;
}
