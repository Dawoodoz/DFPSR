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
#include "../../../base/simd.h"
#include "../../image/PackOrder.h"

namespace dsr {

template<typename U, typename F>
struct Rgba_F32 {
	F red;
	F green;
	F blue;
	F alpha;
	explicit Rgba_F32(const U &color) :
	  red(  floatFromU32(packOrder_getRed(  color))),
	  green(floatFromU32(packOrder_getGreen(color))),
	  blue( floatFromU32(packOrder_getBlue( color))),
	  alpha(floatFromU32(packOrder_getAlpha(color))) {}
	Rgba_F32(const U &color, const PackOrder &order) :
	  red(  floatFromU32(packOrder_getRed(  color, order))),
	  green(floatFromU32(packOrder_getGreen(color, order))),
	  blue( floatFromU32(packOrder_getBlue( color, order))),
	  alpha(floatFromU32(packOrder_getAlpha(color, order))) {}
	Rgba_F32(const F &red, const F &green, const F &blue, const F &alpha) : red(red), green(green), blue(blue), alpha(alpha) {}
	U toSaturatedByte() const {
		return packOrder_floatToSaturatedByte<U, F>(this->red, this->green, this->blue, this->alpha);
	}
	U toSaturatedByte(const PackOrder &order) const {
		return packOrder_floatToSaturatedByte<U, F>(this->red, this->green, this->blue, this->alpha, order);
	}
};

template<typename U, typename F>
inline Rgba_F32<U, F> operator+(const Rgba_F32<U, F> &left, const Rgba_F32<U, F> &right) {
	return Rgba_F32<U, F>(left.red + right.red, left.green + right.green, left.blue + right.blue, left.alpha + right.alpha);
}

template<typename U, typename F>
inline Rgba_F32<U, F> operator-(const Rgba_F32<U, F> &left, const Rgba_F32<U, F> &right) {
	return Rgba_F32<U, F>(left.red - right.red, left.green - right.green, left.blue - right.blue, left.alpha - right.alpha);
}

template<typename U, typename F>
inline Rgba_F32<U, F> operator*(const Rgba_F32<U, F> &left, const Rgba_F32<U, F> &right) {
	return Rgba_F32<U, F>(left.red * right.red, left.green * right.green, left.blue * right.blue, left.alpha * right.alpha);
}

template<typename U, typename F>
inline Rgba_F32<U, F> operator*(const Rgba_F32<U, F> &left, const F &right) {
	return Rgba_F32<U, F>(left.red * right, left.green * right, left.blue * right, left.alpha * right);
}

using Rgba_F32x4 = Rgba_F32<U32x4, F32x4>;
using Rgba_F32x8 = Rgba_F32<U32x8, F32x8>;
using Rgba_F32xX = Rgba_F32<U32xX, F32xX>;

}

#endif
