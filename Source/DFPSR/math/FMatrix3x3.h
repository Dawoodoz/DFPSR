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

#ifndef DFPSR_GEOMETRY_FMATRIX3x3
#define DFPSR_GEOMETRY_FMATRIX3x3

#include <cassert>
#include "FVector.h"

namespace dsr {

struct FMatrix3x3 {
	FVector3D xAxis, yAxis, zAxis;
	FMatrix3x3() :
	  xAxis(FVector3D(1.0f, 0.0f, 0.0f)),
	  yAxis(FVector3D(0.0f, 1.0f, 0.0f)),
	  zAxis(FVector3D(0.0f, 0.0f, 1.0f)) {}
	explicit FMatrix3x3(float uniformScale) :
	  xAxis(FVector3D(uniformScale, 0.0f, 0.0f)),
	  yAxis(FVector3D(0.0f, uniformScale, 0.0f)),
	  zAxis(FVector3D(0.0f, 0.0f, uniformScale)) {}
	FMatrix3x3(const FVector3D &xAxis, const FVector3D &yAxis, const FVector3D &zAxis) :
	  xAxis(xAxis),
	  yAxis(yAxis),
	  zAxis(zAxis) {}
	static FMatrix3x3 makeAxisSystem(const FVector3D &forward, const FVector3D &up) {
		FMatrix3x3 result;
	    FVector3D forwardNormalized = normalize(forward);
		result.zAxis = forwardNormalized;
		result.xAxis = normalize(crossProduct(normalize(up), forwardNormalized));
		result.yAxis = normalize(crossProduct(forwardNormalized, result.xAxis));
		return result;
	}
	// Transform the a vector by multiplying with the matrix
	FVector3D transform(const FVector3D &p) const {
		return FVector3D(
		  p.x * this->xAxis.x + p.y * this->yAxis.x + p.z * this->zAxis.x,
		  p.x * this->xAxis.y + p.y * this->yAxis.y + p.z * this->zAxis.y,
		  p.x * this->xAxis.z + p.y * this->yAxis.z + p.z * this->zAxis.z
		);
	}
	// Transform the a vector by multiplying with the transpose of the matrix
	// The transpose is the inverse for axis aligned normalized matrices
	//   Axis aligned: Each non-self axis dot-product equals zero.
	//   Normalized: The length of each axis equals one.
	FVector3D transformTransposed(const FVector3D &p) const {
		return FVector3D(
		  p.x * this->xAxis.x + p.y * this->xAxis.y + p.z * this->xAxis.z,
		  p.x * this->yAxis.x + p.y * this->yAxis.y + p.z * this->yAxis.z,
		  p.x * this->zAxis.x + p.y * this->zAxis.y + p.z * this->zAxis.z
		);
	}
};

inline FMatrix3x3 operator*(const FMatrix3x3 &m, const float &scale) {
	return FMatrix3x3(m.xAxis * scale, m.yAxis * scale, m.zAxis * scale);
}
inline FMatrix3x3 operator*(const FMatrix3x3 &left, const FMatrix3x3 &right) {
	return FMatrix3x3(right.transform(left.xAxis), right.transform(left.yAxis), right.transform(left.zAxis));
}

inline float determinant(const FMatrix3x3& m) {
	return m.xAxis.x * m.yAxis.y * m.zAxis.z
	     + m.zAxis.x * m.xAxis.y * m.yAxis.z
	     + m.yAxis.x * m.zAxis.y * m.xAxis.z
	     - m.xAxis.x * m.zAxis.y * m.yAxis.z
	     - m.yAxis.x * m.xAxis.y * m.zAxis.z
	     - m.zAxis.x * m.yAxis.y * m.xAxis.z;
}

inline FMatrix3x3 inverseUsingInvDet(const FMatrix3x3& m, float invDet) {
	FMatrix3x3 result;
    result.xAxis.x = invDet * (m.yAxis.y * m.zAxis.z - m.yAxis.z * m.zAxis.y);
    result.xAxis.y = -invDet * (m.xAxis.y * m.zAxis.z - m.xAxis.z * m.zAxis.y);
    result.xAxis.z = invDet * (m.xAxis.y * m.yAxis.z - m.xAxis.z * m.yAxis.y);
    result.yAxis.x = -invDet * (m.yAxis.x * m.zAxis.z - m.yAxis.z * m.zAxis.x);
    result.yAxis.y = invDet * (m.xAxis.x * m.zAxis.z - m.xAxis.z * m.zAxis.x);
    result.yAxis.z = -invDet * (m.xAxis.x * m.yAxis.z - m.xAxis.z * m.yAxis.x);
    result.zAxis.x = invDet * (m.yAxis.x * m.zAxis.y - m.yAxis.y * m.zAxis.x);
    result.zAxis.y = -invDet * (m.xAxis.x * m.zAxis.y - m.xAxis.y * m.zAxis.x);
    result.zAxis.z = invDet * (m.xAxis.x * m.yAxis.y - m.xAxis.y * m.yAxis.x);
	return result;
}

inline FMatrix3x3 inverse(const FMatrix3x3& m) {
	return inverseUsingInvDet(m, 1.0f / determinant(m));
}

}

#endif

