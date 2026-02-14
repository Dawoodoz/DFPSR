// zlib open source license
//
// Copyright (c) 2025 David Forsgren Piuva
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

// Functions used to simplify template programming when using functions both with and without simd.h.

#ifndef DFPSR_NO_SIMD
#define DFPSR_NO_SIMD

#include <stdint.h>
#include <cmath>
#include "SafePointer.h"
#include "DsrTraits.h"
#include <limits>

namespace dsr {
	#define DSR_FLOAT_INF std::numeric_limits<float>::infinity()
	#define DSR_FLOAT_NAN std::numeric_limits<float>::quiet_NaN()
	#define DSR_DOUBLE_INF std::numeric_limits<double>::infinity()
	#define DSR_DOUBLE_NAN std::numeric_limits<double>::quiet_NaN()
	#define DSR_U8_MIN std::numeric_limits<uint8_t>::min()
	#define DSR_U8_MAX std::numeric_limits<uint8_t>::max()
	#define DSR_U16_MIN std::numeric_limits<uint16_t>::min()
	#define DSR_U16_MAX std::numeric_limits<uint16_t>::max()
	#define DSR_U32_MIN std::numeric_limits<uint32_t>::min()
	#define DSR_U32_MAX std::numeric_limits<uint32_t>::max()
	#define DSR_U64_MIN std::numeric_limits<uint64_t>::min()
	#define DSR_U64_MAX std::numeric_limits<uint64_t>::max()
	#define DSR_I16_MIN std::numeric_limits<int16_t>::min()
	#define DSR_I16_MAX std::numeric_limits<int16_t>::max()
	#define DSR_I32_MIN std::numeric_limits<int32_t>::min()
	#define DSR_I32_MAX std::numeric_limits<int32_t>::max()
	#define DSR_I64_MIN std::numeric_limits<int64_t>::min()
	#define DSR_I64_MAX std::numeric_limits<int64_t>::max()

	// Type conversions.
	inline int32_t truncateToI32(float value) { return (int32_t)value; }
	inline uint32_t truncateToU32(float value) { return (uint32_t)value; }
	inline float floatFromI32(int32_t value) { return (float)value; }
	inline float floatFromU32(uint32_t value) { return (float)value; }
	inline int32_t I32FromU32(uint32_t value) { return (int32_t)value; }
	inline uint32_t U32FromI32(int32_t value) { return (uint32_t)value; }

	// Memory read operations.
	inline uint32_t gather_U32(dsr::SafePointer<const uint32_t> data, const uint32_t &elementOffset) { return data[elementOffset]; }
	inline int32_t gather_I32(dsr::SafePointer<const int32_t> data, const uint32_t &elementOffset) { return data[elementOffset]; }
	inline float gather_F32(dsr::SafePointer<const float> data, const uint32_t &elementOffset) { return data[elementOffset]; }

	// Comparisons between all lanes, which is one lane for scalar types.
	inline bool allLanesEqual         (const  uint8_t& left,  const uint8_t& right) { return left == right; }
	inline bool allLanesEqual         (const uint16_t& left, const uint16_t& right) { return left == right; }
	inline bool allLanesEqual         (const uint32_t& left, const uint32_t& right) { return left == right; }
	inline bool allLanesEqual         (const  int32_t& left, const  int32_t& right) { return left == right; }
	inline bool allLanesEqual         (const    float& left, const    float& right) { return abs(left - right) < 0.0001f; }
	inline bool allLanesNotEqual      (const  uint8_t& left, const  uint8_t& right) { return left != right; }
	inline bool allLanesNotEqual      (const uint16_t& left, const uint16_t& right) { return left != right; }
	inline bool allLanesNotEqual      (const uint32_t& left, const uint32_t& right) { return left != right; }
	inline bool allLanesNotEqual      (const  int32_t& left, const  int32_t& right) { return left != right; }
	inline bool allLanesNotEqual      (const    float& left, const    float& right) { return abs(left - right) >= 0.0001f; }
	inline bool allLanesGreater       (const  uint8_t& left, const  uint8_t& right) { return left >  right; }
	inline bool allLanesGreater       (const uint16_t& left, const uint16_t& right) { return left >  right; }
	inline bool allLanesGreater       (const uint32_t& left, const uint32_t& right) { return left >  right; }
	inline bool allLanesGreater       (const  int32_t& left, const  int32_t& right) { return left >  right; }
	inline bool allLanesGreater       (const    float& left, const    float& right) { return left >  right; }
	inline bool allLanesGreaterOrEqual(const  uint8_t& left, const  uint8_t& right) { return left >= right; }
	inline bool allLanesGreaterOrEqual(const uint16_t& left, const uint16_t& right) { return left >= right; }
	inline bool allLanesGreaterOrEqual(const uint32_t& left, const uint32_t& right) { return left >= right; }
	inline bool allLanesGreaterOrEqual(const  int32_t& left, const  int32_t& right) { return left >= right; }
	inline bool allLanesGreaterOrEqual(const    float& left, const    float& right) { return left >= right; }
	inline bool allLanesLesser        (const  uint8_t& left, const  uint8_t& right) { return left <  right; }
	inline bool allLanesLesser        (const uint16_t& left, const uint16_t& right) { return left <  right; }
	inline bool allLanesLesser        (const uint32_t& left, const uint32_t& right) { return left <  right; }
	inline bool allLanesLesser        (const  int32_t& left, const  int32_t& right) { return left <  right; }
	inline bool allLanesLesser        (const    float& left, const    float& right) { return left <  right; }
	inline bool allLanesLesserOrEqual (const  uint8_t& left, const  uint8_t& right) { return left <= right; }
	inline bool allLanesLesserOrEqual (const uint16_t& left, const uint16_t& right) { return left <= right; }
	inline bool allLanesLesserOrEqual (const uint32_t& left, const uint32_t& right) { return left <= right; }
	inline bool allLanesLesserOrEqual (const  int32_t& left, const  int32_t& right) { return left <= right; }
	inline bool allLanesLesserOrEqual (const    float& left, const    float& right) { return left <= right; }

	template <uint32_t bitOffset>
	inline uint32_t bitShiftLeftImmediate(const uint32_t& left) {
		static_assert(bitOffset < 32u, "Immediate left shift of 32-bit values may not shift more than 31 bits!");
		return left << bitOffset;
	}
	template <uint32_t bitOffset>
	inline uint32_t bitShiftRightImmediate(const uint32_t& left) {
		static_assert(bitOffset < 32u, "Immediate right shift of 32-bit values may not shift more than 31 bits!");
		return left >> bitOffset;
	}
	template <uint16_t bitOffset>
	inline uint16_t bitShiftLeftImmediate(const uint16_t& left) {
		static_assert(bitOffset < 16u, "Immediate left shift of 16-bit values may not shift more than 15 bits!");
		return left << bitOffset;
	}
	template <uint16_t bitOffset>
	inline uint16_t bitShiftRightImmediate(const uint16_t& left) {
		static_assert(bitOffset < 16u, "Immediate right shift of 16-bit values may not shift more than 15 bits!");
		return left >> bitOffset;
	}
	template <uint8_t bitOffset>
	inline uint8_t bitShiftLeftImmediate(const uint8_t& left) {
		static_assert(bitOffset < 8u, "Immediate left shift of 8-bit values may not shift more than 7 bits!");
		return left << bitOffset;
	}
	template <uint8_t bitOffset>
	inline uint8_t bitShiftRightImmediate(const uint8_t& left) {
		static_assert(bitOffset < 8u, "Immediate right shift of 8-bit values may not shift more than 7 bits!");
		return left >> bitOffset;
	}

	// A minimum function that can take more than two arguments.
	// Post-condition: Returns the smallest of all given values, which must be comparable using the < operator and have the same type.
	template <typename T, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Scalar, T))>
	inline T min(const T &a, const T &b) {
		return (a < b) ? a : b;
	}
	template <typename T, typename... TAIL, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any, T))>
	inline T min(const T &a, const T &b, TAIL... tail) {
		return min(min(a, b), tail...);
	}

	// A maximum function that can take more than two arguments.
	// Post-condition: Returns the largest of all given values, which must be comparable using the > operator and have the same type.
	template <typename T, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Scalar, T))>
	inline T max(const T &a, const T &b) {
		return (a > b) ? a : b;
	}
	template <typename T, typename... TAIL, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any, T))>
	inline T max(const T &a, const T &b, TAIL... tail) {
		return max(max(a, b), tail...);
	}

	// TODO: Implement min and max for integer vectors in simd.h.
	//       Start by implementing vectorized comparisons and blend functions as a fallback for unsupported types.

	// Post-condition: Returns the absolute value.
	template <typename T, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Scalar, T))>
	inline T abs(const T &value) {
		return max(value, -value);
	}

	// Post-condition: Returns abs(a - b)
	template <typename T, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Scalar, T))>
	inline T absDiff(const T &a, const T &b) {
		return (a > b) ? (a - b) : (b - a);
	}

	// Pre-condition: minValue <= maxValue
	// Post-condition: Returns value clamped from minValue to maxValue.
	template <typename T, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any, T))>
	inline T clamp(const T &minValue, const T &value, const T &maxValue) {
		return max(minValue, min(value, maxValue));
	}

	// Post-condition: Returns value clamped to minValue.
	template <typename T, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any, T))>
	inline T clampLower(const T &minValue, const T &value) {
		return max(minValue, value);
	}

	// Post-condition: Returns value clamped to maxValue.
	template <typename T, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any, T))>
	inline T clampUpper(const T &value, const T &maxValue) {
		return min(value, maxValue);
	}

	inline float reciprocal(float value) { return 1.0f / value; }

	inline float reciprocalSquareRoot(float value) { return 1.0f / sqrt(value); }

	inline float squareRoot(float value) { return sqrt(value); }

	// TODO: Add more functions from simd.h.
}

#endif
