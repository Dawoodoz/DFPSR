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

#ifndef DFPSR_MATH_SCALAR
#define DFPSR_MATH_SCALAR

#include <cmath>

namespace dsr {

// Preconditions:
//   0 <= a <= 255
//   0 <= b <= 255
// Postconditions:
//   Returns the normalized multiplication of a and b, where the 0..255 range represents decimal values from 0.0 to 1.0.
//   The result may not be less than zero or larger than any of the inputs.
// Examples:
//   mulByte_8(0, 0) = 0
//   mulByte_8(x, 0) = 0
//   mulByte_8(0, x) = 0
//   mulByte_8(x, 255) = x
//   mulByte_8(255, x) = x
//   mulByte_8(255, 255) = 255
static inline uint32_t mulByte_8(uint32_t a, uint32_t b) {
	// Approximate the reciprocal of an unsigned byte's maximum value 255 for normalization
	//   256³ / 255 ≈ 65793
	// Truncation goes down, so add half a unit before rounding to get the closest value
	//   2^24 / 2 = 8388608
	// No overflow for unsigned 32-bit integers
	//   255² * 65793 + 8388608 = 4286578433 < 2^32
	return (a * b * 65793 + 8388608) >> 24;
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

// True iff high and low bytes are equal
//   Equivalent to value % 257 == 0 because A + B * 256 = A * 257 when A = B.
inline bool isUniformByteU16(uint16_t value) {
	return (value & 0x00FF) == ((value & 0xFF00) >> 8);
}

// A special rounding used for triangle rasterization
inline int64_t safeRoundInt64(float value) {
	int64_t result = floor(value);
	if (value <= -1048576.0f || value >= 1048576.0f) { result = 0; }
	if (value < 0.0f) { result--; }
	return result;
}

}

#endif

