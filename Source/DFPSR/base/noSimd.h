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
#include "SafePointer.h"

namespace dsr {
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
		static_assert(bitOffset < 32u);
		return left << bitOffset;
	}
	template <uint32_t bitOffset>
	inline uint32_t bitShiftRightImmediate(const uint32_t& left) {
		static_assert(bitOffset < 32u);
		return left >> bitOffset;
	}
	template <uint16_t bitOffset>
	inline uint16_t bitShiftLeftImmediate(const uint16_t& left) {
		static_assert(bitOffset < 16u);
		return left << bitOffset;
	}
	template <uint16_t bitOffset>
	inline uint16_t bitShiftRightImmediate(const uint16_t& left) {
		static_assert(bitOffset < 16u);
		return left >> bitOffset;
	}
	template <uint8_t bitOffset>
	inline uint8_t bitShiftLeftImmediate(const uint8_t& left) {
		static_assert(bitOffset < 8u);
		return left << bitOffset;
	}
	template <uint8_t bitOffset>
	inline uint8_t bitShiftRightImmediate(const uint8_t& left) {
		static_assert(bitOffset < 8u);
		return left >> bitOffset;
	}

	// TODO: Add more functions from simd.h.
}

#endif
