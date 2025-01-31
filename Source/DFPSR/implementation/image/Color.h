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

#ifndef DFPSR_IMAGE_COLOR
#define DFPSR_IMAGE_COLOR

#include <cstdint>
#include "../../api/stringAPI.h"

namespace dsr {

struct ColorRgbI32;
struct ColorRgbaI32;
inline ColorRgbI32 operator * (const ColorRgbI32& left, float right);
inline ColorRgbI32 operator * (const ColorRgbI32& left, int32_t right);
inline ColorRgbI32 operator + (const ColorRgbI32& left, const ColorRgbI32& right);
inline bool operator == (const ColorRgbI32& a, const ColorRgbI32& b);
inline bool operator != (const ColorRgbI32& a, const ColorRgbI32& b);
inline ColorRgbaI32 operator *( const ColorRgbaI32& left, float right);
inline ColorRgbaI32 operator * (const ColorRgbaI32& left, int32_t right);
inline ColorRgbaI32 operator + (const ColorRgbaI32& left, const ColorRgbaI32& right);
inline bool operator == (const ColorRgbaI32& a, const ColorRgbaI32& b);
inline bool operator != (const ColorRgbaI32& a, const ColorRgbaI32& b);

// RGB color with 32 bits per channel
//   Values outside of the 0..255 byte range may cause unexpected behaviour
struct ColorRgbI32 {
	int32_t red, green, blue;
	ColorRgbI32() : red(0), green(0), blue(0) {}
	explicit ColorRgbI32(int32_t uniform) : red(uniform), green(uniform), blue(uniform) {}
	ColorRgbI32(int32_t red, int32_t green, int32_t blue) : red(red), green(green), blue(blue) {}
	// Get the color clamped to the visible range.
	ColorRgbI32 saturate() const {
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
	static ColorRgbI32 mix(const ColorRgbI32& colorA, const ColorRgbI32& colorB, float weight) {
		float invWeight = 1.0f - weight;
		return (colorA * invWeight) + (colorB * weight);
	}
	// Create a color from a string.
	explicit ColorRgbI32(const ReadableString &content) : red(0), green(0), blue(0) {
		int givenChannels = 0;
		string_split_callback([this, &givenChannels](ReadableString channelValue) {
			if (givenChannels == 0) {
				this->red = string_toInteger(channelValue);
			} else if (givenChannels == 1) {
				this->green = string_toInteger(channelValue);
			} else if (givenChannels == 2) {
				this->blue = string_toInteger(channelValue);
			}
			givenChannels++;
		}, content, U',');
	}
};
inline ColorRgbI32 operator * (const ColorRgbI32& left, float right) {
	return ColorRgbI32((float)left.red * right, (float)left.green * right, (float)left.blue * right);
}
inline ColorRgbI32 operator * (const ColorRgbI32& left, int32_t right) {
	return ColorRgbI32(left.red * right, left.green * right, left.blue * right);
}
inline ColorRgbI32 operator + (const ColorRgbI32& left, const ColorRgbI32& right) {
	return ColorRgbI32(left.red + right.red, left.green + right.green, left.blue + right.blue);
}
inline bool operator == (const ColorRgbI32& a, const ColorRgbI32& b) {
	return a.red == b.red && a.green == b.green && a.blue == b.blue;
}
inline bool operator != (const ColorRgbI32& a, const ColorRgbI32& b) {
	return !(a == b);
}

// RGBA color with 32 bits per channel
//   Values outside of the 0..255 byte range may cause unexpected behaviour
struct ColorRgbaI32 {
	int32_t red, green, blue, alpha;
	ColorRgbaI32() : red(0), green(0), blue(0), alpha(0) {}
	ColorRgbaI32(ColorRgbI32 rgb, int32_t alpha) : red(rgb.red), green(rgb.green), blue(rgb.blue), alpha(alpha) {}
	explicit ColorRgbaI32(int32_t uniform) : red(uniform), green(uniform), blue(uniform), alpha(uniform) {}
	ColorRgbaI32(int32_t red, int32_t green, int32_t blue, int32_t alpha) : red(red), green(green), blue(blue), alpha(alpha) {}
	// Get the color clamped to the visible range.
	ColorRgbaI32 saturate() const {
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
	static ColorRgbaI32 mix(const ColorRgbaI32& colorA, const ColorRgbaI32& colorB, float weight) {
		float invWeight = 1.0f - weight;
		return (colorA * invWeight) + (colorB * weight);
	}
	// Create a color from a string.
	explicit ColorRgbaI32(const ReadableString &content) : red(0), green(0), blue(0), alpha(255) {
		int givenChannels = 0;
		string_split_callback([this, &givenChannels](ReadableString channelValue) {
			if (givenChannels == 0) {
				this->red = string_toInteger(channelValue);
			} else if (givenChannels == 1) {
				this->green = string_toInteger(channelValue);
			} else if (givenChannels == 2) {
				this->blue = string_toInteger(channelValue);
			} else if (givenChannels == 3) {
				this->alpha = string_toInteger(channelValue);
			}
			givenChannels++;
		}, content, U',');
	}
};
inline ColorRgbaI32 operator *( const ColorRgbaI32& left, float right) {
	return ColorRgbaI32((float)left.red * right, (float)left.green * right, (float)left.blue * right, (float)left.alpha * right);
}
inline ColorRgbaI32 operator * (const ColorRgbaI32& left, int32_t right) {
	return ColorRgbaI32(left.red * right, left.green * right, left.blue * right, left.alpha * right);
}
inline ColorRgbaI32 operator + (const ColorRgbaI32& left, const ColorRgbaI32& right) {
	return ColorRgbaI32(left.red + right.red, left.green + right.green, left.blue + right.blue, left.alpha + right.alpha);
}
inline bool operator == (const ColorRgbaI32& a, const ColorRgbaI32& b) {
	return a.red == b.red && a.green == b.green && a.blue == b.blue && a.alpha == b.alpha;
}
inline bool operator != (const ColorRgbaI32& a, const ColorRgbaI32& b) {
	return !(a == b);
}

// Serialization
inline String& string_toStreamIndented(String& target, const ColorRgbI32& source, const ReadableString& indentation) {
	string_append(target, indentation, source.red, U",", source.green, U",", source.blue);
	return target;
}
inline String& string_toStreamIndented(String& target, const ColorRgbaI32& source, const ReadableString& indentation) {
	string_append(target, indentation, source.red, U",", source.green, U",", source.blue, U",", source.alpha);
	return target;
}

}

#endif

