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

#ifndef DFPSR_MATH_SCALAR
#define DFPSR_MATH_SCALAR

#include "../base/noSimd.h"

namespace dsr {

// Returns a modulo b where 0 <= a < b
template <typename I, typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Scalar_SignedInteger, I) && DSR_CHECK_PROPERTY(DsrTrait_Scalar_Integer, U))>
inline int32_t signedModulo(I a, U b) {
	if (a >= 0) {
		return a % b; // Simple modulo
	} else {
		return (b - (-a % b)) % b; // Negative modulo
	}
}

template <typename I, typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Scalar_SignedInteger, I) && DSR_CHECK_PROPERTY(DsrTrait_Scalar_Integer, U))>
inline I roundUp(I size, U alignment) {
	return size + (alignment - 1) - signedModulo(size - 1, alignment);
}

template <typename I, typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Scalar_SignedInteger, I) && DSR_CHECK_PROPERTY(DsrTrait_Scalar_Integer, U))>
inline I roundDown(I size, U alignment) {
	return size - signedModulo(size, alignment);
}

template <typename T, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Scalar_Floating, T))>
inline T absDiff(T a, T b) {
	float result = a - b;
	if (result < 0.0f) {
		result = -result;
	}
	return result;
}

inline uint8_t absDiff(uint8_t a, uint8_t b) {
	int32_t result = (int32_t)a - (int32_t)b;
	if (result < 0) {
		result = -result;
	}
	return (uint8_t)result;
}

inline uint16_t absDiff(uint16_t a, uint16_t b) {
	int32_t result = (int32_t)a - (int32_t)b;
	if (result < 0) {
		result = -result;
	}
	return (uint16_t)result;
}

// Only use this for trivial types, use std::swap for objects with non-trivial construction.
template <typename T>
inline void swap(T &a, T &b) {
	T temp = a;
	a = b;
	b = temp;
}

// More compact than min(a, b) when reading from the target
template <typename T, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Scalar, T))>
inline void replaceWithSmaller(T &target, const T &source) {
	if (source < target) {
		target = source;
	}
}

// More compact than max(a, b) when reading from the target
template <typename T, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Scalar, T))>
inline void replaceWithLarger(T &target, const T &source) {
	if (source > target) {
		target = source;
	}
}

}

#endif
