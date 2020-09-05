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

#ifndef DFPSR_GEOMETRY_FVECTOR
#define DFPSR_GEOMETRY_FVECTOR

#include "vectorMethods.h"

namespace dsr {

struct FVector2D {
	VECTOR_BODY_2D(FVector2D, float, 0.0f);
};
struct FVector3D {
	VECTOR_BODY_3D(FVector3D, float, 0.0f);
};
struct FVector4D {
	VECTOR_BODY_4D(FVector4D, float, 0.0f);
};

OPERATORS_2D(FVector2D, float);
OPERATORS_3D(FVector3D, float);
OPERATORS_4D(FVector4D, float);
SIGNED_OPERATORS_2D(FVector2D, float);
SIGNED_OPERATORS_3D(FVector3D, float);
SIGNED_OPERATORS_4D(FVector4D, float);
SERIALIZATION_2D(FVector2D);
SERIALIZATION_3D(FVector3D);
SERIALIZATION_4D(FVector4D);

inline bool operator==(const FVector2D &left, const FVector2D &right) {
	return fabs(left.x - right.x) < 0.0001f && fabs(left.y - right.y) < 0.0001f;
}
inline bool operator==(const FVector3D &left, const FVector3D &right) {
	return fabs(left.x - right.x) < 0.0001f && fabs(left.y - right.y) < 0.0001f && fabs(left.z - right.z) < 0.0001f;
}
inline bool operator==(const FVector4D &left, const FVector4D &right) {
	return fabs(left.x - right.x) < 0.0001f && fabs(left.y - right.y) < 0.0001f && fabs(left.z - right.z) < 0.0001f && fabs(left.w - right.w) < 0.0001f;
}

inline bool operator!=(const FVector2D &left, const FVector2D &right) {
	return !(left == right);
}
inline bool operator!=(const FVector3D &left, const FVector3D &right) {
	return !(left == right);
}
inline bool operator!=(const FVector4D &left, const FVector4D &right) {
	return !(left == right);
}

inline float dotProduct(const FVector2D &a, const FVector2D &b) {
	return (a.x * b.x) + (a.y * b.y);
}
inline float dotProduct(const FVector3D &a, const FVector3D &b) {
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}
inline float dotProduct(const FVector4D &a, const FVector4D &b) {
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
}

inline float squareLength(const FVector2D &v) {
	return v.x * v.x + v.y * v.y;
}
inline float squareLength(const FVector3D &v) {
	return v.x * v.x + v.y * v.y + v.z * v.z;
}
inline float squareLength(const FVector4D &v) {
	return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

inline float length(const FVector2D &v) {
	return sqrtf(squareLength(v));
}
inline float length(const FVector3D &v) {
	return sqrtf(squareLength(v));
}
inline float length(const FVector4D &v) {
	return sqrtf(squareLength(v));
}

inline FVector3D crossProduct(const FVector3D &a, const FVector3D &b) {
	return FVector3D(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

inline FVector2D normalize(const FVector2D &v) {
	float l = length(v);
	if (l == 0.0f) {
		return FVector2D(0.0f, 1.0f);
	} else {
		return v / length(v);
	}
}
inline FVector3D normalize(const FVector3D &v) {
	float l = length(v);
	if (l == 0.0f) {
		return FVector3D(0.0f, 0.0f, 1.0f);
	} else {
		return v / length(v);
	}
}
inline FVector4D normalize(const FVector4D &v) {
	float l = length(v);
	if (l == 0.0f) {
		return FVector4D(0.0f, 0.0f, 0.0f, 1.0f);
	} else {
		return v / length(v);
	}
}

}

#endif

