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

#include <cmath>

namespace dsr {

// A minimum function that can take more than two arguments.
// Post-condition: Returns the smallest of all given values, which must be comparable using the < operator and have the same type.
template <typename T>
inline T min(const T &a, const T &b) {
	return (a < b) ? a : b;
}
template <typename T, typename... TAIL>
inline T min(const T &a, const T &b, TAIL... tail) {
	return min(min(a, b), tail...);
}

// A maximum function that can take more than two arguments.
// Post-condition: Returns the largest of all given values, which must be comparable using the > operator and have the same type.
template <typename T>
inline T max(const T &a, const T &b) {
	return (a > b) ? a : b;
}
template <typename T, typename... TAIL>
inline T max(const T &a, const T &b, TAIL... tail) {
	return max(max(a, b), tail...);
}

// Returns a modulo b where 0 <= a < b
inline int signedModulo(int a, int b) {
	int result = 0;
	if (b > 0) {
		if (a >= 0) {
			result = a % b; // Simple modulo
		} else {
			result = (b - (-a % b)) % b; // Negative modulo
		}
	}
	return result;
}

inline int roundUp(int size, int alignment) {
	return size + (alignment - 1) - signedModulo(size - 1, alignment);
}

inline int roundDown(int size, int alignment) {
	return size - signedModulo(size, alignment);
}

inline float absDiff(float a, float b) {
	float result = a - b;
	if (result < 0.0f) {
		result = -result;
	}
	return result;
}

inline uint8_t absDiff(uint8_t a, uint8_t b) {
	int result = (int)a - (int)b;
	if (result < 0) {
		result = -result;
	}
	return (uint8_t)result;
}

inline uint16_t absDiff(uint16_t a, uint16_t b) {
	int result = (int)a - (int)b;
	if (result < 0) {
		result = -result;
	}
	return (uint16_t)result;
}

// Allowing compilation on older C++ versions
// Only use for trivial types if you want to avoid cloning and destruction
template <typename T>
inline void swap(T &a, T &b) {
	T temp = a;
	a = b;
	b = temp;
}

// More compact than min(a, b) when reading from the target
template <typename T>
inline void replaceWithSmaller(T& target, T source) {
	if (source < target) {
		target = source;
	}
}

// More compact than max(a, b) when reading from the target
template <typename T>
inline void replaceWithLarger(T& target, T source) {
	if (source > target) {
		target = source;
	}
}

}

#endif

