// zlib open source license
//
// Copyright (c) 2017 to 2023 David Forsgren Piuva
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

namespace dsr {

// These are the infix operations for 2D SIMD vectors F32x4x2, F32x8x2...
#define SIMD_VECTOR_INFIX_OPERATORS_2D(VECTOR_TYPE, SIMD_TYPE, ELEMENT_TYPE) \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.v1 + right.v1, left.v2 + right.v2); \
} \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, const SIMD_TYPE &right) { \
	return VECTOR_TYPE(left.v1 + right, left.v2 + right); \
} \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, const ELEMENT_TYPE &right) { \
	return VECTOR_TYPE(left.v1 + right, left.v2 + right); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.v1 - right.v1, left.v2 - right.v2); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, const SIMD_TYPE &right) { \
	return VECTOR_TYPE(left.v1 - right, left.v2 - right); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, const ELEMENT_TYPE &right) { \
	return VECTOR_TYPE(left.v1 - right, left.v2 - right); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE& value) { \
	return VECTOR_TYPE(-value.v1, -value.v2); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.v1 * right.v1, left.v2 * right.v2); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, const SIMD_TYPE &right) { \
	return VECTOR_TYPE(left.v1 * right, left.v2 * right); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, const ELEMENT_TYPE &right) { \
	return VECTOR_TYPE(left.v1 * right, left.v2 * right); \
} \
inline SIMD_TYPE dotProduct(const VECTOR_TYPE &a, const VECTOR_TYPE &b) { \
	return (a.v1 * b.v1) + (a.v2 * b.v2); \
} \
inline SIMD_TYPE squareLength(const VECTOR_TYPE &v) { \
	return dotProduct(v, v); \
} \
inline SIMD_TYPE length(const VECTOR_TYPE &v) { \
	return squareRoot(squareLength(v)); \
} \
inline VECTOR_TYPE normalize(const VECTOR_TYPE &v) { \
	return v * reciprocalSquareRoot(squareLength(v)); \
}

// These are the infix operations for 3D SIMD vectors F32x4x3, F32x8x3...
#define SIMD_VECTOR_INFIX_OPERATORS_3D(VECTOR_TYPE, SIMD_TYPE, ELEMENT_TYPE) \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.v1 + right.v1, left.v2 + right.v2, left.v3 + right.v3); \
} \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, const SIMD_TYPE &right) { \
	return VECTOR_TYPE(left.v1 + right, left.v2 + right, left.v3 + right); \
} \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, const ELEMENT_TYPE &right) { \
	return VECTOR_TYPE(left.v1 + right, left.v2 + right, left.v3 + right); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.v1 - right.v1, left.v2 - right.v2, left.v3 - right.v3); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, const SIMD_TYPE &right) { \
	return VECTOR_TYPE(left.v1 - right, left.v2 - right, left.v3 - right); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, const ELEMENT_TYPE &right) { \
	return VECTOR_TYPE(left.v1 - right, left.v2 - right, left.v3 - right); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE& value) { \
	return VECTOR_TYPE(-value.v1, -value.v2, -value.v3); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.v1 * right.v1, left.v2 * right.v2, left.v3 * right.v3); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, const SIMD_TYPE &right) { \
	return VECTOR_TYPE(left.v1 * right, left.v2 * right, left.v3 * right); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, const ELEMENT_TYPE &right) { \
	return VECTOR_TYPE(left.v1 * right, left.v2 * right, left.v3 * right); \
} \
inline SIMD_TYPE dotProduct(const VECTOR_TYPE &a, const VECTOR_TYPE &b) { \
	return (a.v1 * b.v1) + (a.v2 * b.v2) + (a.v3 * b.v3); \
} \
inline SIMD_TYPE squareLength(const VECTOR_TYPE &v) { \
	return dotProduct(v, v); \
} \
inline SIMD_TYPE length(const VECTOR_TYPE &v) { \
	return squareRoot(squareLength(v)); \
} \
inline VECTOR_TYPE normalize(const VECTOR_TYPE &v) { \
	return v * reciprocalSquareRoot(squareLength(v)); \
}

// These are the available in-plaxe operations for 2D SIMD vectors F32x4x2, F32x8x2...
#define SIMD_VECTOR_MEMBER_OPERATORS_2D(VECTOR_TYPE, SIMD_TYPE, ELEMENT_TYPE) \
	inline VECTOR_TYPE& operator+=(const VECTOR_TYPE& offset) { this->v1 = this->v1 + offset.v1; this->v2 = this->v2 + offset.v2; return *this; } \
	inline VECTOR_TYPE& operator-=(const VECTOR_TYPE& offset) { this->v1 = this->v1 - offset.v1; this->v2 = this->v2 - offset.v2; return *this; } \
	inline VECTOR_TYPE& operator*=(const VECTOR_TYPE& scale) { this->v1 = this->v1 * scale.v1; this->v2 = this->v2 * scale.v2; return *this; } \
	inline VECTOR_TYPE& operator+=(const SIMD_TYPE& offset) { this->v1 = this->v1 + offset; this->v2 = this->v2 + offset; return *this; } \
	inline VECTOR_TYPE& operator-=(const SIMD_TYPE& offset) { this->v1 = this->v1 - offset; this->v2 = this->v2 - offset; return *this; } \
	inline VECTOR_TYPE& operator*=(const SIMD_TYPE& scale) { this->v1 = this->v1 * scale; this->v2 = this->v2 * scale; return *this; } \
	inline VECTOR_TYPE& operator+=(const ELEMENT_TYPE& offset) { this->v1 = this->v1 + offset; this->v2 = this->v2 + offset; return *this; } \
	inline VECTOR_TYPE& operator-=(const ELEMENT_TYPE& offset) { this->v1 = this->v1 - offset; this->v2 = this->v2 - offset; return *this; } \
	inline VECTOR_TYPE& operator*=(const ELEMENT_TYPE& scale) { this->v1 = this->v1 * scale; this->v2 = this->v2 * scale; return *this; }

// These are the available in-plaxe operations for 3D SIMD vectors F32x4x3, F32x8x3...
#define SIMD_VECTOR_MEMBER_OPERATORS_3D(VECTOR_TYPE, SIMD_TYPE, ELEMENT_TYPE) \
	inline VECTOR_TYPE& operator+=(const VECTOR_TYPE& offset) { this->v1 = this->v1 + offset.v1; this->v2 = this->v2 + offset.v2; this->v3 = this->v3 + offset.v3; return *this; } \
	inline VECTOR_TYPE& operator-=(const VECTOR_TYPE& offset) { this->v1 = this->v1 - offset.v1; this->v2 = this->v2 - offset.v2; this->v3 = this->v3 - offset.v3; return *this; } \
	inline VECTOR_TYPE& operator*=(const VECTOR_TYPE& scale) { this->v1 = this->v1 * scale.v1; this->v2 = this->v2 * scale.v2; this->v3 = this->v3 * scale.v3; return *this; } \
	inline VECTOR_TYPE& operator+=(const SIMD_TYPE& offset) { this->v1 = this->v1 + offset; this->v2 = this->v2 + offset; this->v3 = this->v3 + offset; return *this; } \
	inline VECTOR_TYPE& operator-=(const SIMD_TYPE& offset) { this->v1 = this->v1 - offset; this->v2 = this->v2 - offset; this->v3 = this->v3 - offset; return *this; } \
	inline VECTOR_TYPE& operator*=(const SIMD_TYPE& scale) { this->v1 = this->v1 * scale; this->v2 = this->v2 * scale; this->v3 = this->v3 * scale; return *this; } \
	inline VECTOR_TYPE& operator+=(const ELEMENT_TYPE& offset) { this->v1 = this->v1 + offset; this->v2 = this->v2 + offset; this->v3 = this->v3 + offset; return *this; } \
	inline VECTOR_TYPE& operator-=(const ELEMENT_TYPE& offset) { this->v1 = this->v1 - offset; this->v2 = this->v2 - offset; this->v3 = this->v3 - offset; return *this; } \
	inline VECTOR_TYPE& operator*=(const ELEMENT_TYPE& scale) { this->v1 = this->v1 * scale; this->v2 = this->v2 * scale; this->v3 = this->v3 * scale; return *this; }

// 128x2-bit SIMD vectorized 2D math vector stored in xxxxyyyy format (one planar SIMD vector per dimension).
struct F32x4x2 {
	F32x4 v1, v2;
	// Direct constructor given 3 rows of length 4
	F32x4x2(const F32x4& v1, const F32x4& v2)
	: v1(v1), v2(v2) {}
	// Gradient constructor from an initial vector and the increment for each element.
	static F32x4x2 createGradient(const dsr::FVector3D& start, const dsr::FVector3D& increment) {
		return F32x4x2(
		  F32x4::createGradient(start.x, increment.x),
		  F32x4::createGradient(start.y, increment.y)
		);
	}
	// Transposed constructor given 4 columns of length 2 (Only allowed for fixed size SIMD, not X or F vector lengths)
	F32x4x2(const dsr::FVector2D& a, const dsr::FVector2D& b, const dsr::FVector2D& c, const dsr::FVector2D& d)
	: v1(a.x, b.x, c.x, d.x),
	  v2(a.y, b.y, c.y, d.y) {}
	// Transposed constructor given a single repeated column
	F32x4x2(const dsr::FVector2D& v)
	: v1(F32x4(v.x)),
	  v2(F32x4(v.y)) {}
	// In-place math operations
	SIMD_VECTOR_MEMBER_OPERATORS_2D(F32x4x2, F32x4, float)
};
SIMD_VECTOR_INFIX_OPERATORS_2D(F32x4x2, F32x4, float)

// 256x2-bit SIMD vectorized 2D math vector stored in xxxxxxxxyyyyyyyy format (one planar SIMD vector per dimension).
struct F32x8x2 {
	F32x8 v1, v2;
	// Direct constructor given 3 rows of length 4
	F32x8x2(const F32x8& v1, const F32x8& v2)
	: v1(v1), v2(v2) {}
	// Gradient constructor from an initial vector and the increment for each element.
	static F32x8x2 createGradient(const dsr::FVector3D& start, const dsr::FVector3D& increment) {
		return F32x8x2(
		  F32x8::createGradient(start.x, increment.x),
		  F32x8::createGradient(start.y, increment.y)
		);
	}
	// Transposed constructor given 4 columns of length 2 (Only allowed for fixed size SIMD, not X or F vector lengths)
	F32x8x2(const dsr::FVector2D& a, const dsr::FVector2D& b, const dsr::FVector2D& c, const dsr::FVector2D& d, const dsr::FVector2D& e, const dsr::FVector2D& f, const dsr::FVector2D& g, const dsr::FVector2D& h)
	: v1(a.x, b.x, c.x, d.x, e.x, f.x, g.x, h.x),
	  v2(a.y, b.y, c.y, d.y, e.y, f.y, g.y, h.y) {}
	// Transposed constructor given a single repeated column
	F32x8x2(const dsr::FVector2D& v)
	: v1(F32x8(v.x)),
	  v2(F32x8(v.y)) {}
	// In-place math operations
	SIMD_VECTOR_MEMBER_OPERATORS_2D(F32x8x2, F32x8, float)
};
SIMD_VECTOR_INFIX_OPERATORS_2D(F32x8x2, F32x8, float)

// 128x3-bit SIMD vectorized 3D math vector stored in xxxxyyyyzzzz format (one planar SIMD vector per dimension).
struct F32x4x3 {
	F32x4 v1, v2, v3;
	// Direct constructor given 3 rows of length 4
	F32x4x3(const F32x4& v1, const F32x4& v2, const F32x4& v3)
	: v1(v1), v2(v2), v3(v3) {}
	// Gradient constructor from an initial vector and the increment for each element.
	static F32x4x3 createGradient(const dsr::FVector3D& start, const dsr::FVector3D& increment) {
		return F32x4x3(
		  F32x4::createGradient(start.x, increment.x),
		  F32x4::createGradient(start.y, increment.y),
		  F32x4::createGradient(start.z, increment.z)
		);
	}
	// Transposed constructor given 4 columns of length 3
	F32x4x3(const dsr::FVector3D& a, const dsr::FVector3D& b, const dsr::FVector3D& c, const dsr::FVector3D& d)
	: v1(a.x, b.x, c.x, d.x),
	  v2(a.y, b.y, c.y, d.y),
	  v3(a.z, b.z, c.z, d.z) {}
	// Transposed constructor given a single repeated column
	F32x4x3(const dsr::FVector3D& v)
	: v1(F32x4(v.x)),
	  v2(F32x4(v.y)),
	  v3(F32x4(v.z)) {}
	// In-place math operations
	SIMD_VECTOR_MEMBER_OPERATORS_3D(F32x4x3, F32x4, float)
};
SIMD_VECTOR_INFIX_OPERATORS_3D(F32x4x3, F32x4, float)

// 256x3-bit SIMD vectorized 3D math vector stored in xxxxxxxxyyyyyyyyzzzzzzzz format (one planar SIMD vector per dimension).
struct F32x8x3 {
	F32x8 v1, v2, v3;
	// Direct constructor given 3 rows of length 4
	F32x8x3(const F32x8& v1, const F32x8& v2, const F32x8& v3)
	: v1(v1), v2(v2), v3(v3) {}
	// Gradient constructor from an initial vector and the increment for each element.
	static F32x8x3 createGradient(const dsr::FVector3D& start, const dsr::FVector3D& increment) {
		return F32x8x3(
		  F32x8::createGradient(start.x, increment.x),
		  F32x8::createGradient(start.y, increment.y),
		  F32x8::createGradient(start.z, increment.z)
		);
	}
	// Transposed constructor given 4 columns of length 3
	F32x8x3(const dsr::FVector3D& a, const dsr::FVector3D& b, const dsr::FVector3D& c, const dsr::FVector3D& d, const dsr::FVector3D& e, const dsr::FVector3D& f, const dsr::FVector3D& g, const dsr::FVector3D& h)
	: v1(a.x, b.x, c.x, d.x, e.x, f.x, g.x, h.x),
	  v2(a.y, b.y, c.y, d.y, e.y, f.y, g.y, h.y),
	  v3(a.z, b.z, c.z, d.z, e.z, f.z, g.z, h.z) {}
	// Transposed constructor given a single repeated column
	F32x8x3(const dsr::FVector3D& v)
	: v1(F32x8(v.x)),
	  v2(F32x8(v.y)),
	  v3(F32x8(v.z)) {}
	// In-place math operations
	SIMD_VECTOR_MEMBER_OPERATORS_3D(F32x8x3, F32x8, float)
};
SIMD_VECTOR_INFIX_OPERATORS_3D(F32x8x3, F32x8, float)

// X vector aliases
#if DSR_DEFAULT_VECTOR_SIZE == 16
	using F32xXx3 = F32x4x3;
	using F32xXx2 = F32x4x2;
#elif DSR_DEFAULT_VECTOR_SIZE == 32
	using F32xXx3 = F32x8x3;
	using F32xXx2 = F32x8x2;
#endif

// F vector aliases
#if DSR_FLOAT_VECTOR_SIZE == 16
	using F32xFx3 = F32x4x3;
	using F32xFx2 = F32x4x2;
#elif DSR_FLOAT_VECTOR_SIZE == 32
	using F32xFx3 = F32x8x3;
	using F32xFx2 = F32x8x2;
#endif

#undef SIMD_VECTOR_MEMBER_OPERATORS_2D
#undef SIMD_VECTOR_MEMBER_OPERATORS_3D

}

#endif

