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

#include <stdint.h>
#include "../base/text.h"

namespace dsr {

// RGB color with 32 bits per channel
//   Values outside of the 0..255 byte range may cause unexpected behaviour
struct ColorRgbI32 {
	int32_t red, green, blue;
	ColorRgbI32() : red(0), green(0), blue(0) {}
	explicit ColorRgbI32(int32_t uniform) : red(uniform), green(uniform), blue(uniform) {}
	ColorRgbI32(int32_t red, int32_t green, int32_t blue) : red(red), green(green), blue(blue) {}
	// Clamp to the valid range
	ColorRgbI32 saturate() const;
	static ColorRgbI32 mix(const ColorRgbI32& colorA, const ColorRgbI32& colorB, float weight);
	// Create a color from a string
	explicit ColorRgbI32(const ReadableString &content);
};
inline ColorRgbI32 operator*(const ColorRgbI32& left, float right) {
	return ColorRgbI32((float)left.red * right, (float)left.green * right, (float)left.blue * right);
}
inline ColorRgbI32 operator*(const ColorRgbI32& left, int32_t right) {
	return ColorRgbI32(left.red * right, left.green * right, left.blue * right);
}
inline ColorRgbI32 operator+(const ColorRgbI32& left, const ColorRgbI32& right) {
	return ColorRgbI32(left.red + right.red, left.green + right.green, left.blue + right.blue);
}
inline bool operator== (const ColorRgbI32& a, const ColorRgbI32& b) {
	return a.red == b.red && a.green == b.green && a.blue == b.blue;
}
inline bool operator!= (const ColorRgbI32& a, const ColorRgbI32& b) {
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
	// Clamp to the valid range
	ColorRgbaI32 saturate() const;
	static ColorRgbaI32 mix(const ColorRgbaI32& colorA, const ColorRgbaI32& colorB, float weight);
	// Create a color from a string
	explicit ColorRgbaI32(const ReadableString &content);
};
inline ColorRgbaI32 operator*(const ColorRgbaI32& left, float right) {
	return ColorRgbaI32((float)left.red * right, (float)left.green * right, (float)left.blue * right, (float)left.alpha * right);
}
inline ColorRgbaI32 operator*(const ColorRgbaI32& left, int32_t right) {
	return ColorRgbaI32(left.red * right, left.green * right, left.blue * right, left.alpha * right);
}
inline ColorRgbaI32 operator+(const ColorRgbaI32& left, const ColorRgbaI32& right) {
	return ColorRgbaI32(left.red + right.red, left.green + right.green, left.blue + right.blue, left.alpha + right.alpha);
}
inline bool operator== (const ColorRgbaI32& a, const ColorRgbaI32& b) {
	return a.red == b.red && a.green == b.green && a.blue == b.blue && a.alpha == b.alpha;
}
inline bool operator!= (const ColorRgbaI32& a, const ColorRgbaI32& b) {
	return !(a == b);
}

// TODO: Can this type be hidden from the external API?
// RGBA color in arbitrary pack order for speed
// Use ImageRgbaU8Impl::packRgba to construct for a specific pack order
union Color4xU8 {
	uint32_t packed;
	uint8_t channels[4];
	Color4xU8() : packed(0) {}
	explicit Color4xU8(uint32_t packed) : packed(packed) {}
	Color4xU8(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth) : channels{first, second, third, fourth} {}
	bool isUniformByte() {
		int first = this->channels[0];
		return this->channels[1] == first && this->channels[2] == first && this->channels[3] == first;
	}
};
inline bool operator== (const Color4xU8& a, const Color4xU8& b) {
	return a.packed == b.packed;
}
inline bool operator!= (const Color4xU8& a, const Color4xU8& b) {
	return !(a == b);
}

// Serialization
String& string_toStreamIndented(String& target, const ColorRgbI32& source, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const ColorRgbaI32& source, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const Color4xU8& source, const ReadableString& indentation);

}

#endif

