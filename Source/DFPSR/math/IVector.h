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

#ifndef DFPSR_GEOMETRY_IVECTOR
#define DFPSR_GEOMETRY_IVECTOR

#include "vectorMethods.h"

namespace dsr {

struct IVector2D {
	VECTOR_BODY_2D(IVector2D, int32_t, 0);
};
struct IVector3D {
	VECTOR_BODY_3D(IVector3D, int32_t, 0);
};
struct IVector4D {
	VECTOR_BODY_4D(IVector4D, int32_t, 0);
};

inline int32_t dotProduct(const IVector2D &a, const IVector2D &b) {
	return (a.x * b.x) + (a.y * b.y);
}
inline int32_t dotProduct(const IVector3D &a, const IVector3D &b) {
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}
inline int32_t dotProduct(const IVector4D &a, const IVector4D &b) {
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

inline IVector3D crossProduct(const IVector3D &a, const IVector3D &b) {
	return IVector3D(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

OPERATORS_2D(IVector2D, int32_t);
OPERATORS_3D(IVector3D, int32_t);
OPERATORS_4D(IVector4D, int32_t);
SIGNED_OPERATORS_2D(IVector2D, int32_t);
SIGNED_OPERATORS_3D(IVector3D, int32_t);
SIGNED_OPERATORS_4D(IVector4D, int32_t);
EXACT_COMPARE_2D(IVector2D);
EXACT_COMPARE_3D(IVector3D);
EXACT_COMPARE_4D(IVector4D);
SERIALIZATION_2D(IVector2D);
SERIALIZATION_3D(IVector3D);
SERIALIZATION_4D(IVector4D);

}

#endif

