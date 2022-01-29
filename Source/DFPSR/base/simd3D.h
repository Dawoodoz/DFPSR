// zlib open source license
//
// Copyright (c) 2017 to 2022 David Forsgren Piuva
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

#include "simd.h"
#include "../math/FVector.h"

// Linear algebra of up to three dimensions. For operating on four unrelated vectors in parallel.
//   Unlike simd.h, this is not a hardware abstraction layer using assembly intrinsics directly.
//   This module builds on top of simd.h for higher levels of abstraction.

#ifndef DFPSR_SIMD_3D
#define DFPSR_SIMD_3D

// 3D vector in xxxxyyyyzzzz format
struct F32x4x3 {
	F32x4 v1, v2, v3;
	// Direct constructor given 3 rows of length 4
	F32x4x3(const F32x4& v1, const F32x4& v2, const F32x4& v3)
	: v1(v1), v2(v2), v3(v3) {}
	// Transposed constructor given 4 columns of length 3
	F32x4x3(const dsr::FVector3D& vx, const dsr::FVector3D& vy, const dsr::FVector3D& vz, const dsr::FVector3D& vw)
	: v1(F32x4(vx.x, vy.x, vz.x, vw.x)),
	  v2(F32x4(vx.y, vy.y, vz.y, vw.y)),
	  v3(F32x4(vx.z, vy.z, vz.z, vw.z)) {}
	// Transposed constructor given a single repeated column
	F32x4x3(const dsr::FVector3D& v)
	: v1(F32x4(v.x, v.x, v.x, v.x)),
	  v2(F32x4(v.y, v.y, v.y, v.y)),
	  v3(F32x4(v.z, v.z, v.z, v.z)) {}
	// In-place math operations
	inline F32x4x3& operator+=(const F32x4x3& offset) { this->v1 = this->v1 + offset.v1; this->v2 = this->v2 + offset.v2; this->v3 = this->v3 + offset.v3; return *this; }
	inline F32x4x3& operator-=(const F32x4x3& offset) { this->v1 = this->v1 - offset.v1; this->v2 = this->v2 - offset.v2; this->v3 = this->v3 - offset.v3; return *this; }
	inline F32x4x3& operator*=(const F32x4x3& scale) { this->v1 = this->v1 * scale.v1; this->v2 = this->v2 * scale.v2; this->v3 = this->v3 * scale.v3; return *this; }
	inline F32x4x3& operator+=(const F32x4& offset) { this->v1 = this->v1 + offset; this->v2 = this->v2 + offset; this->v3 = this->v3 + offset; return *this; }
	inline F32x4x3& operator-=(const F32x4& offset) { this->v1 = this->v1 - offset; this->v2 = this->v2 - offset; this->v3 = this->v3 - offset; return *this; }
	inline F32x4x3& operator*=(const F32x4& scale) { this->v1 = this->v1 * scale; this->v2 = this->v2 * scale; this->v3 = this->v3 * scale; return *this; }
	inline F32x4x3& operator+=(const float& offset) { this->v1 = this->v1 + offset; this->v2 = this->v2 + offset; this->v3 = this->v3 + offset; return *this; }
	inline F32x4x3& operator-=(const float& offset) { this->v1 = this->v1 - offset; this->v2 = this->v2 - offset; this->v3 = this->v3 - offset; return *this; }
	inline F32x4x3& operator*=(const float& scale) { this->v1 = this->v1 * scale; this->v2 = this->v2 * scale; this->v3 = this->v3 * scale; return *this; }
};

inline F32x4x3 operator+(const F32x4x3 &left, const F32x4x3 &right) {
	return F32x4x3(left.v1 + right.v1, left.v2 + right.v2, left.v3 + right.v3);
}
inline F32x4x3 operator+(const F32x4x3 &left, const F32x4 &right) {
	return F32x4x3(left.v1 + right, left.v2 + right, left.v3 + right);
}
inline F32x4x3 operator+(const F32x4x3 &left, const float &right) {
	return F32x4x3(left.v1 + right, left.v2 + right, left.v3 + right);
}

inline F32x4x3 operator-(const F32x4x3 &left, const F32x4x3 &right) {
	return F32x4x3(left.v1 - right.v1, left.v2 - right.v2, left.v3 - right.v3);
}
inline F32x4x3 operator-(const F32x4x3 &left, const F32x4 &right) {
	return F32x4x3(left.v1 - right, left.v2 - right, left.v3 - right);
}
inline F32x4x3 operator-(const F32x4x3 &left, const float &right) {
	return F32x4x3(left.v1 - right, left.v2 - right, left.v3 - right);
}
inline F32x4x3 operator-(const F32x4x3& value) {
	return F32x4x3(-value.v1, -value.v2, -value.v3);
}

inline F32x4x3 operator*(const F32x4x3 &left, const F32x4x3 &right) {
	return F32x4x3(left.v1 * right.v1, left.v2 * right.v2, left.v3 * right.v3);
}
inline F32x4x3 operator*(const F32x4x3 &left, const F32x4 &right) {
	return F32x4x3(left.v1 * right, left.v2 * right, left.v3 * right);
}
inline F32x4x3 operator*(const F32x4x3 &left, const float &right) {
	return F32x4x3(left.v1 * right, left.v2 * right, left.v3 * right);
}

inline F32x4 dotProduct(const F32x4x3 &a, const F32x4x3 &b) {
	return (a.v1 * b.v1) + (a.v2 * b.v2) + (a.v3 * b.v3);
}

inline F32x4 squareLength(const F32x4x3 &v) {
	return dotProduct(v, v);
}

inline F32x4 length(const F32x4x3 &v) {
	return squareLength(v).squareRoot();
}

inline F32x4x3 normalize(const F32x4x3 &v) {
	return v * squareLength(v).reciprocalSquareRoot();
}

// 2D vector in xxxxyyyy format
struct F32x4x2 {
	F32x4 v1, v2;
	// Direct constructor given 3 rows of length 4
	F32x4x2(const F32x4& v1, const F32x4& v2)
	: v1(v1), v2(v2) {}
	// Transposed constructor given 4 columns of length 3
	F32x4x2(const dsr::FVector2D& vx, const dsr::FVector2D& vy, const dsr::FVector2D& vz, const dsr::FVector2D& vw)
	: v1(F32x4(vx.x, vy.x, vz.x, vw.x)),
	  v2(F32x4(vx.y, vy.y, vz.y, vw.y)) {}
	// Transposed constructor given a single repeated column
	F32x4x2(const dsr::FVector2D& v)
	: v1(F32x4(v.x, v.x, v.x, v.x)),
	  v2(F32x4(v.y, v.y, v.y, v.y)) {}
	// In-place math operations
	inline F32x4x2& operator+=(const F32x4x2& offset) { this->v1 = this->v1 + offset.v1; this->v2 = this->v2 + offset.v2; return *this; }
	inline F32x4x2& operator-=(const F32x4x2& offset) { this->v1 = this->v1 - offset.v1; this->v2 = this->v2 - offset.v2; return *this; }
	inline F32x4x2& operator*=(const F32x4x2& scale) { this->v1 = this->v1 * scale.v1; this->v2 = this->v2 * scale.v2; return *this; }
	inline F32x4x2& operator+=(const F32x4& offset) { this->v1 = this->v1 + offset; this->v2 = this->v2 + offset; return *this; }
	inline F32x4x2& operator-=(const F32x4& offset) { this->v1 = this->v1 - offset; this->v2 = this->v2 - offset; return *this; }
	inline F32x4x2& operator*=(const F32x4& scale) { this->v1 = this->v1 * scale; this->v2 = this->v2 * scale; return *this; }
	inline F32x4x2& operator+=(const float& offset) { this->v1 = this->v1 + offset; this->v2 = this->v2 + offset; return *this; }
	inline F32x4x2& operator-=(const float& offset) { this->v1 = this->v1 - offset; this->v2 = this->v2 - offset; return *this; }
	inline F32x4x2& operator*=(const float& scale) { this->v1 = this->v1 * scale; this->v2 = this->v2 * scale; return *this; }
};

inline F32x4x2 operator+(const F32x4x2 &left, const F32x4x2 &right) {
	return F32x4x2(left.v1 + right.v1, left.v2 + right.v2);
}
inline F32x4x2 operator+(const F32x4x2 &left, const F32x4 &right) {
	return F32x4x2(left.v1 + right, left.v2 + right);
}
inline F32x4x2 operator+(const F32x4x2 &left, const float &right) {
	return F32x4x2(left.v1 + right, left.v2 + right);
}

inline F32x4x2 operator-(const F32x4x2 &left, const F32x4x2 &right) {
	return F32x4x2(left.v1 - right.v1, left.v2 - right.v2);
}
inline F32x4x2 operator-(const F32x4x2 &left, const F32x4 &right) {
	return F32x4x2(left.v1 - right, left.v2 - right);
}
inline F32x4x2 operator-(const F32x4x2 &left, const float &right) {
	return F32x4x2(left.v1 - right, left.v2 - right);
}
inline F32x4x2 operator-(const F32x4x2& value) {
	return F32x4x2(-value.v1, -value.v2);
}

inline F32x4x2 operator*(const F32x4x2 &left, const F32x4x2 &right) {
	return F32x4x2(left.v1 * right.v1, left.v2 * right.v2);
}
inline F32x4x2 operator*(const F32x4x2 &left, const F32x4 &right) {
	return F32x4x2(left.v1 * right, left.v2 * right);
}
inline F32x4x2 operator*(const F32x4x2 &left, const float &right) {
	return F32x4x2(left.v1 * right, left.v2 * right);
}

inline F32x4 dotProduct(const F32x4x2 &a, const F32x4x2 &b) {
	return (a.v1 * b.v1) + (a.v2 * b.v2);
}

inline F32x4 squareLength(const F32x4x2 &v) {
	return dotProduct(v, v);
}

inline F32x4 length(const F32x4x2 &v) {
	return squareLength(v).squareRoot();
}

inline F32x4x2 normalize(const F32x4x2 &v) {
	return v * squareLength(v).reciprocalSquareRoot();
}

#endif

