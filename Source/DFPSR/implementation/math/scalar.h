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

#include "../../base/noSimd.h"

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

// Substitute for std::move.
template<typename T>
T&& move(T& source) {
	// Cast from l-value reference (&) to r-value reference (&&), as a way of saying that the source is used as a temporary expression that can be moved from.
	// Then the result will match with move instead of copy when the result is used for assignment or construction.
	return static_cast<T&&>(source);
}

// Substitute for std::swap.
template <typename T>
inline void swap(T &a, T &b) {
	T temp = dsr::move(a);
	a = dsr::move(b);
	b = dsr::move(temp);
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
