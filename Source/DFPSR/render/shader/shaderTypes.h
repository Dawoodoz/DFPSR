// zlib open source license
//
// Copyright (c) 2017 to 2019 David Forsgren Piuva
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

#ifndef DFPSR_RENDER_SHADER_TYPES
#define DFPSR_RENDER_SHADER_TYPES

#include <cstdint>
#include <cstdio>
#include "../../base/simd.h"
#include "../../image/PackOrder.h"

namespace dsr {

struct Rgba_F32 {
	F32x4 red;
	F32x4 green;
	F32x4 blue;
	F32x4 alpha;
	explicit Rgba_F32(const U32x4 &color) :
	  red(  floatFromU32(getRed(  color))),
	  green(floatFromU32(getGreen(color))),
	  blue( floatFromU32(getBlue( color))),
	  alpha(floatFromU32(getAlpha(color))) {}
	Rgba_F32(const U32x4 &color, const PackOrder &order) :
	  red(  floatFromU32(getRed(  color, order))),
	  green(floatFromU32(getGreen(color, order))),
	  blue( floatFromU32(getBlue( color, order))),
	  alpha(floatFromU32(getAlpha(color, order))) {}
	Rgba_F32(const F32x4 &red, const F32x4 &green, const F32x4 &blue, const F32x4 &alpha) : red(red), green(green), blue(blue), alpha(alpha) {}
	// TODO: Use a template argument for deciding the packing order for external image formats
	U32x4 toSaturatedByte() const {
		return floatToSaturatedByte(this->red, this->green, this->blue, this->alpha);
	}
	U32x4 toSaturatedByte(const PackOrder &order) const {
		return floatToSaturatedByte(this->red, this->green, this->blue, this->alpha, order);
	}
};
inline Rgba_F32 operator+(const Rgba_F32 &left, const Rgba_F32 &right) {
	return Rgba_F32(left.red + right.red, left.green + right.green, left.blue + right.blue, left.alpha + right.alpha);
}
inline Rgba_F32 operator-(const Rgba_F32 &left, const Rgba_F32 &right) {
	return Rgba_F32(left.red - right.red, left.green - right.green, left.blue - right.blue, left.alpha - right.alpha);
}
inline Rgba_F32 operator*(const Rgba_F32 &left, const Rgba_F32 &right) {
	return Rgba_F32(left.red * right.red, left.green * right.green, left.blue * right.blue, left.alpha * right.alpha);
}
inline Rgba_F32 operator*(const Rgba_F32 &left, const F32x4 &right) {
	return Rgba_F32(left.red * right, left.green * right, left.blue * right, left.alpha * right);
}

}

#endif


