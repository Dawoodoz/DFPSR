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

#ifndef DFPSR_GEOMETRY_TRANSFORM3D
#define DFPSR_GEOMETRY_TRANSFORM3D

#include "FVector.h"
#include "FMatrix3x3.h"

namespace dsr {

class Transform3D {
public:
	FVector3D position;
	FMatrix3x3 transform;
	Transform3D() : position(0.0f, 0.0f, 0.0f), transform(FVector3D(1.0f, 0.0f, 0.0f), FVector3D(0.0f, 1.0f, 0.0f), FVector3D(0.0f, 0.0f, 1.0f)) {}
	Transform3D(const FVector3D &position, const FMatrix3x3 &transform) :
	  position(position),
	  transform(transform) {}

	// Transform the point by multiplying with the 3x3 matrix and adding the translation
	FVector3D transformPoint(const FVector3D &p) const {
		return this->transform.transform(p) + this->position;
	}
	// Transform the vector by multiplying with the 3x3 matrix
	FVector3D transformVector(const FVector3D &p) const {
		return this->transform.transform(p);
	}
	// Transform the a vector by multiplying with the transpose of the 3x3 matrix
	// The transpose is the inverse for axis aligned normalized matrices
	// Precondition: The transform must be normalized and axis aligned (Allows rotation but no shear nor scaling)
	FVector3D transformPointTransposedInverse(const FVector3D &p) const {
		return this->transform.transformTransposed(p - this->position);
	}
};

inline Transform3D operator*(const Transform3D &left, const Transform3D &right) {
	return Transform3D(right.transformPoint(left.position), left.transform * right.transform);
}

// The determinant of a transform is the volume of a cube transformed by the matrix.
//   Inside-out transforms have a negative volume. (mirrored by negating one axis or swapping two)
inline float determinant(const Transform3D& m) {
	return determinant(m.transform);
}

inline Transform3D inverseUsingInvDet(const Transform3D& m, float invDet) {
	Transform3D result;
    result.transform.xAxis.x = invDet * (m.transform.yAxis.y * m.transform.zAxis.z - m.transform.yAxis.z * m.transform.zAxis.y);
    result.transform.xAxis.y = -invDet * (m.transform.xAxis.y * m.transform.zAxis.z - m.transform.xAxis.z * m.transform.zAxis.y);
    result.transform.xAxis.z = invDet * (m.transform.xAxis.y * m.transform.yAxis.z - m.transform.xAxis.z * m.transform.yAxis.y);
    result.transform.yAxis.x = -invDet * (m.transform.yAxis.x * m.transform.zAxis.z - m.transform.yAxis.z * m.transform.zAxis.x);
    result.transform.yAxis.y = invDet * (m.transform.xAxis.x * m.transform.zAxis.z - m.transform.xAxis.z * m.transform.zAxis.x);
    result.transform.yAxis.z = -invDet * (m.transform.xAxis.x * m.transform.yAxis.z - m.transform.xAxis.z * m.transform.yAxis.x);
    result.transform.zAxis.x = invDet * (m.transform.yAxis.x * m.transform.zAxis.y - m.transform.yAxis.y * m.transform.zAxis.x);
    result.transform.zAxis.y = -invDet * (m.transform.xAxis.x * m.transform.zAxis.y - m.transform.xAxis.y * m.transform.zAxis.x);
    result.transform.zAxis.z = invDet * (m.transform.xAxis.x * m.transform.yAxis.y - m.transform.xAxis.y * m.transform.yAxis.x);
    result.position.x = -(m.position.x * result.transform.xAxis.x + m.position.y * result.transform.yAxis.x + m.position.z * result.transform.zAxis.x);
    result.position.y = -(m.position.x * result.transform.xAxis.y + m.position.y * result.transform.yAxis.y + m.position.z * result.transform.zAxis.y);
    result.position.z = -(m.position.x * result.transform.xAxis.z + m.position.y * result.transform.yAxis.z + m.position.z * result.transform.zAxis.z);
	return result;
}

inline Transform3D inverse(const Transform3D& m) {
	return inverseUsingInvDet(m, 1.0f / determinant(m));
}

}

#endif

