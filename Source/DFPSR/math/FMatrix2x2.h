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

#ifndef DFPSR_GEOMETRY_FMATRIX2x2
#define DFPSR_GEOMETRY_FMATRIX2x2

#include <cassert>
#include "FVector.h"

namespace dsr {

struct FMatrix2x2 {
	FVector2D xAxis, yAxis;
	FMatrix2x2() :
	  xAxis(FVector2D(1.0f, 0.0f)),
	  yAxis(FVector2D(0.0f, 1.0f)) {}
	explicit FMatrix2x2(float uniformScale) :
	  xAxis(FVector2D(uniformScale, 0.0f)),
	  yAxis(FVector2D(0.0f, uniformScale)) {}
	FMatrix2x2(const FVector2D &xAxis, const FVector2D &yAxis) :
	  xAxis(xAxis),
	  yAxis(yAxis) {}
	// Transform the a vector by multiplying with the matrix
	FVector2D transform(const FVector2D &p) const {
		return FVector2D(
		  p.x * this->xAxis.x + p.y * this->yAxis.x,
		  p.x * this->xAxis.y + p.y * this->yAxis.y
		);
	}
	// Transform the a vector by multiplying with the transpose of the matrix
	// The transpose is the inverse for axis aligned normalized matrices
	//   Axis aligned: Each non-self axis dot-product equals zero.
	//   Normalized: The length of each axis equals one.
	FVector2D transformTransposed(const FVector2D &p) const {
		return FVector2D(
		  p.x * this->xAxis.x + p.y * this->xAxis.y,
		  p.x * this->yAxis.x + p.y * this->yAxis.y
		);
	}
};

inline FMatrix2x2 operator*(const FMatrix2x2 &m, const float &scale) {
	return FMatrix2x2(m.xAxis * scale, m.yAxis * scale);
}
inline FMatrix2x2 operator*(const FMatrix2x2 &left, const FMatrix2x2 &right) {
	return FMatrix2x2(right.transform(left.xAxis), right.transform(left.yAxis));
}

inline float determinant(const FMatrix2x2& m) {
	return m.xAxis.x * m.yAxis.y - m.xAxis.y * m.yAxis.x;
}

// The full matrix inverse for any matrix where the determinant is not zero
inline FMatrix2x2 inverse(const FMatrix2x2& m) {
	return FMatrix2x2(FVector2D(m.yAxis.y, -m.xAxis.y), FVector2D(-m.yAxis.x, m.xAxis.x)) * (1.0f / determinant(m));
}

inline String& string_toStreamIndented(String& target, const FMatrix2x2& source, const ReadableString& indentation) {
	string_append(target, indentation, U"XAxis(", source.xAxis, U"), YAxis(", source.yAxis, U")");
	return target;
}

}

#endif

