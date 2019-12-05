
// zlib open source license
//
// Copyright (c) 2019 David Forsgren Piuva
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

#ifndef DFPSR_FIXED_POINT
#define DFPSR_FIXED_POINT

#include "../base/text.h"

namespace dsr {

// One extra unit in early clamping allows fractions to extend the range further
// int16_t goes from -32768 to +32767, but when having additional fractions, one can get close to the -32769 to 32768 range
inline void clampForSaturatedWhole(int64_t& value) {
	if (value > 32768) { value = 32768; }
	if (value < -32769) { value = -32769; }
}
inline void clampForInt32(int64_t& value) {
	if (value > 2147483647) { value = 2147483647; }
	if (value < -2147483648) { value = -2147483648; }
}

// A deterministic saturated fixed point number for graphics and virtual machines.
//   Uses 16-bits for whole signed integers and 16-bits for the remaining 1/65536 fractions.
//   The fromMantissa constructor can be used to store 32-bit indices directly in the mantissa.
//     If used as a value, the index is taken as 1/65536 fractions.
//     Retreive correctly using getMantissa.
//   Default initialized to zero for convenience.
struct FixedPoint {
private:
	int32_t mantissa = 0;
public:
	FixedPoint();
	// TODO: Can comparisons use an implicit conversion from whole integers to reduce complexity?
	explicit FixedPoint(int64_t newMantissa);
	static FixedPoint fromWhole(int64_t wholeInteger);
	static FixedPoint fromMantissa(int64_t mantissa);
	static FixedPoint zero();
	static FixedPoint epsilon();
	static FixedPoint half();
	static FixedPoint one();
	static FixedPoint fromText(const ReadableString& content);
	inline int64_t getMantissa() const {
		return (int64_t)this->mantissa;
	}
};

String& string_toStreamIndented(String& target, const FixedPoint& value, const ReadableString& indentation);

// Addition and subtraction is faster against its own type, by being in the same scale
inline FixedPoint operator+(const FixedPoint &left, const FixedPoint &right) {
	return FixedPoint(left.getMantissa() + right.getMantissa());
}
inline FixedPoint operator+(const FixedPoint &left, int32_t right) {
	return FixedPoint(left.getMantissa() + (right * 65536));
}
inline FixedPoint operator+(int32_t left, const FixedPoint &right) {
	return FixedPoint((left * 65536) + right.getMantissa());
}
inline FixedPoint operator-(const FixedPoint &left, const FixedPoint &right) {
	return FixedPoint(left.getMantissa() - right.getMantissa());
}
inline FixedPoint operator-(const FixedPoint &left, int32_t right) {
	return FixedPoint(left.getMantissa() - (right * 65536));
}
inline FixedPoint operator-(int32_t left, const FixedPoint &right) {
	return FixedPoint((left * 65536) - right.getMantissa());
}

// Multiplication is faster against whole integers, by not having to reduce the result
inline FixedPoint operator*(const FixedPoint &left, const FixedPoint &right) {
	return FixedPoint((left.getMantissa() * right.getMantissa()) / 65536);
}
inline FixedPoint operator*(const FixedPoint &left, int64_t right) {
	clampForSaturatedWhole(right);
	return FixedPoint(left.getMantissa() * right);
}
inline FixedPoint operator*(int64_t left, const FixedPoint &right) {
	clampForSaturatedWhole(left);
	return FixedPoint(left * right.getMantissa());
}

int32_t fixedPoint_round(const FixedPoint& value);
double fixedPoint_approximate(const FixedPoint& value);

FixedPoint fixedPoint_min(const FixedPoint &left, const FixedPoint &right);
FixedPoint fixedPoint_max(const FixedPoint &left, const FixedPoint &right);
FixedPoint fixedPoint_divide(const FixedPoint &left, const FixedPoint &right);
FixedPoint fixedPoint_divide(const FixedPoint &left, int64_t right);
inline FixedPoint operator/(const FixedPoint &left, const FixedPoint &right) {
	return fixedPoint_divide(left, right);
}
inline FixedPoint operator/(const FixedPoint &left, int64_t right) {
	return fixedPoint_divide(left, right);
}
inline FixedPoint operator/(int64_t left, const FixedPoint &right) {
	return fixedPoint_divide(FixedPoint::fromWhole(left), right);
}

// Gets the real element of value's square root.
//   Because square roots of negative numbers are only using the imaginary dimension, this results in zero for all non-positive inputs.
FixedPoint fixedPoint_squareRoot(const FixedPoint& value);

inline bool operator==(const FixedPoint &left, const FixedPoint &right) {
	return left.getMantissa() == right.getMantissa();
}
inline bool operator==(const FixedPoint &left, int64_t right) {
	return left.getMantissa() == right * 65536;
}
inline bool operator==(int64_t left, const FixedPoint &right) {
	return left * 65536 == right.getMantissa();
}
inline bool operator!=(const FixedPoint &left, const FixedPoint &right) {
	return left.getMantissa() != right.getMantissa();
}
inline bool operator!=(const FixedPoint &left, int64_t right) {
	return left.getMantissa() != right * 65536;
}
inline bool operator!=(int64_t left, const FixedPoint &right) {
	return left * 65536 != right.getMantissa();
}
inline bool operator>(const FixedPoint &left, const FixedPoint &right) {
	return left.getMantissa() > right.getMantissa();
}
inline bool operator>(const FixedPoint &left, int64_t right) {
	return left.getMantissa() > right * 65536;
}
inline bool operator>(int64_t left, const FixedPoint &right) {
	return left * 65536 > right.getMantissa();
}
inline bool operator<(const FixedPoint &left, const FixedPoint &right) {
	return left.getMantissa() < right.getMantissa();
}
inline bool operator<(const FixedPoint &left, int64_t right) {
	return left.getMantissa() < right * 65536;
}
inline bool operator<(int64_t left, const FixedPoint &right) {
	return left * 65536 < right.getMantissa();
}
inline bool operator>=(const FixedPoint &left, const FixedPoint &right) {
	return left.getMantissa() >= right.getMantissa();
}
inline bool operator>=(const FixedPoint &left, int64_t right) {
	return left.getMantissa() >= right * 65536;
}
inline bool operator>=(int64_t left, const FixedPoint &right) {
	return left * 65536 >= right.getMantissa();
}
inline bool operator<=(const FixedPoint &left, const FixedPoint &right) {
	return left.getMantissa() <= right.getMantissa();
}
inline bool operator<=(const FixedPoint &left, int64_t right) {
	return left.getMantissa() <= right * 65536;
}
inline bool operator<=(int64_t left, const FixedPoint &right) {
	return left * 65536 <= right.getMantissa();
}

// TODO: Equality and other comparisons

}

#endif

