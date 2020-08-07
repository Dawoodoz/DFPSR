// zlib open source license
//
// Copyright (c) 2018 to 2019 David Forsgren Piuva
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

#include "Color.h"

using namespace dsr;

ColorRgbI32 ColorRgbI32::saturate() const {
	int32_t red = this->red;
	int32_t green = this->green;
	int32_t blue = this->blue;
	if (red < 0) { red = 0; }
	if (red > 255) { red = 255; }
	if (green < 0) { green = 0; }
	if (green > 255) { green = 255; }
	if (blue < 0) { blue = 0; }
	if (blue > 255) { blue = 255; }
	return ColorRgbI32(red, green, blue);
}
ColorRgbI32 ColorRgbI32::mix(const ColorRgbI32& colorA, const ColorRgbI32& colorB, float weight) {
	float invWeight = 1.0f - weight;
	return (colorA * invWeight) + (colorB * weight);
}
ColorRgbI32::ColorRgbI32(const ReadableString &content) : red(0), green(0), blue(0) {
	List<ReadableString> elements = string_split(content, U',');
	int givenChannels = elements.length();
	if (givenChannels >= 1) {
		this-> red = string_parseInteger(elements[0]);
		if (givenChannels >= 2) {
			this-> green = string_parseInteger(elements[1]);
			if (givenChannels >= 3) {
				this-> blue = string_parseInteger(elements[2]);
			}
		}
	}
}
ColorRgbaI32 ColorRgbaI32::saturate() const {
	int32_t red = this->red;
	int32_t green = this->green;
	int32_t blue = this->blue;
	int32_t alpha = this->alpha;
	if (red < 0) { red = 0; }
	if (red > 255) { red = 255; }
	if (green < 0) { green = 0; }
	if (green > 255) { green = 255; }
	if (blue < 0) { blue = 0; }
	if (blue > 255) { blue = 255; }
	if (alpha < 0) { alpha = 0; }
	if (alpha > 255) { alpha = 255; }
	return ColorRgbaI32(red, green, blue, alpha);
}
ColorRgbaI32 ColorRgbaI32::mix(const ColorRgbaI32& colorA, const ColorRgbaI32& colorB, float weight) {
	float invWeight = 1.0f - weight;
	return (colorA * invWeight) + (colorB * weight);
}
ColorRgbaI32::ColorRgbaI32(const ReadableString &content) : red(0), green(0), blue(0), alpha(255) {
	List<ReadableString> elements = string_split(content, U',');
	int givenChannels = elements.length();
	if (givenChannels >= 1) {
		this-> red = string_parseInteger(elements[0]);
		if (givenChannels >= 2) {
			this-> green = string_parseInteger(elements[1]);
			if (givenChannels >= 3) {
				this-> blue = string_parseInteger(elements[2]);
				if (givenChannels >= 4) {
					this-> alpha = string_parseInteger(elements[3]);
				}
			}
		}
	}
}

String& dsr::string_toStreamIndented(String& target, const ColorRgbI32& source, const ReadableString& indentation) {
	string_append(target, indentation, source.red, U",", source.green, U",", source.blue);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const ColorRgbaI32& source, const ReadableString& indentation) {
	string_append(target, indentation, source.red, U",", source.green, U",", source.blue, U",", source.alpha);
	return target;
}
String& dsr::string_toStreamIndented(String& target, const Color4xU8& source, const ReadableString& indentation) {
	string_append(target, indentation, source.channels[0], U",", source.channels[1], U",", source.channels[2], U",", source.channels[3]);
	return target;
}
