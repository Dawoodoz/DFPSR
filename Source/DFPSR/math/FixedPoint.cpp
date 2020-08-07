
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

#include "FixedPoint.h"
#include <cmath> // Only use the methods guaranteed to be exact, unless an approximation is requested

using namespace dsr;

/* This sum of 0.9999999999999999999 explains why including the 20:th decimal would cause overflow from rounding to closest.
16602069666338596454 + 1660206966633859645 // 0.9 + 0.09
18262276632972456099 + 166020696663385965 // 0.99 + 0.009
18428297329635842064 + 16602069666338596 // 0.999 + 0.0009
18444899399302180660 + 1660206966633860 // 0.9999 + 0.00009
18446559606268814520 + 166020696663386 // 0.99999 + 0.000009
18446725626965477906 + 16602069666339 // 0.999999 + 0.0000009
18446742229035144245 + 1660206966634 // 0.9999999 + 0.00000009
18446743889242110879 + 166020696663 // 0.99999999 + 0.000000009
18446744055262807542 + 16602069666 // 0.999999999 + 0.0000000009
18446744071864877208 + 1660206967 // 0.9999999999 + 0.00000000009
18446744073525084175 + 166020697 // 0.99999999999 + 0.000000000009
18446744073691104872 + 16602070 // 0.999999999999 + 0.0000000000009
18446744073707706942 + 1660207 // 0.9999999999999 + 0.00000000000009
18446744073709367149 + 166021 // 0.99999999999999 + 0.000000000000009
18446744073709533170 + 16602 // 0.999999999999999 + 0.0000000000000009
18446744073709549772 + 1660 // 0.9999999999999999 + 0.00000000000000009
18446744073709551432 + 166 // 0.99999999999999999 + 0.000000000000000009
18446744073709551598 + 17 // 0.999999999999999999 + 0.0000000000000000009
18446744073709551615     // 0.9999999999999999999
18446744073709551616    // 1.0
*/

// Including the 20:th decimal would cause overflow from rounding to closest.
const int maxDecimals = 19;
// Each group of 9 values contains the digit fractions for a certain location
static const uint64_t decimalFractions64[maxDecimals * 9] = {
	// Calculated using the Wolfram expression "round(18446744073709551616 * 1 / 10)" et cetera...
	UINT64_C( 1844674407370955162), // 2^64 * 0.1
	UINT64_C( 3689348814741910323), // 2^64 * 0.2
	UINT64_C( 5534023222112865485), // 2^64 * 0.3
	UINT64_C( 7378697629483820646), // 2^64 * 0.4
	UINT64_C( 9223372036854775808), // 2^64 * 0.5
	UINT64_C(11068046444225730970), // 2^64 * 0.6
	UINT64_C(12912720851596686131), // 2^64 * 0.7
	UINT64_C(14757395258967641293), // 2^64 * 0.8
	UINT64_C(16602069666338596454), // 2^64 * 0.9
	UINT64_C( 184467440737095516), // 2^64 * 0.01
	UINT64_C( 368934881474191032), // 2^64 * 0.02
	UINT64_C( 553402322211286548), // 2^64 * 0.03
	UINT64_C( 737869762948382065), // 2^64 * 0.04
	UINT64_C( 922337203685477581), // 2^64 * 0.05
	UINT64_C(1106804644422573097), // 2^64 * 0.06
	UINT64_C(1291272085159668613), // 2^64 * 0.07
	UINT64_C(1475739525896764129), // 2^64 * 0.08
	UINT64_C(1660206966633859645), // 2^64 * 0.09
	UINT64_C( 18446744073709552), // 2^64 * 0.001
	UINT64_C( 36893488147419103), // 2^64 * 0.002
	UINT64_C( 55340232221128655), // 2^64 * 0.003
	UINT64_C( 73786976294838206), // 2^64 * 0.004
	UINT64_C( 92233720368547758), // 2^64 * 0.005
	UINT64_C(110680464442257310), // 2^64 * 0.006
	UINT64_C(129127208515966861), // 2^64 * 0.007
	UINT64_C(147573952589676413), // 2^64 * 0.008
	UINT64_C(166020696663385965), // 2^64 * 0.009
	UINT64_C( 1844674407370955), // 2^64 * 0.0001
	UINT64_C( 3689348814741910), // 2^64 * 0.0002
	UINT64_C( 5534023222112865), // 2^64 * 0.0003
	UINT64_C( 7378697629483821), // 2^64 * 0.0004
	UINT64_C( 9223372036854776), // 2^64 * 0.0005
	UINT64_C(11068046444225731), // 2^64 * 0.0006
	UINT64_C(12912720851596686), // 2^64 * 0.0007
	UINT64_C(14757395258967641), // 2^64 * 0.0008
	UINT64_C(16602069666338596), // 2^64 * 0.0009
	UINT64_C( 184467440737096), // 2^64 * 0.00001
	UINT64_C( 368934881474191), // 2^64 * 0.00002
	UINT64_C( 553402322211287), // 2^64 * 0.00003
	UINT64_C( 737869762948382), // 2^64 * 0.00004
	UINT64_C( 922337203685478), // 2^64 * 0.00005
	UINT64_C(1106804644422573), // 2^64 * 0.00006
	UINT64_C(1291272085159669), // 2^64 * 0.00007
	UINT64_C(1475739525896764), // 2^64 * 0.00008
	UINT64_C(1660206966633860), // 2^64 * 0.00009
	UINT64_C( 18446744073710), // 2^64 * 0.000001
	UINT64_C( 36893488147419), // 2^64 * 0.000002
	UINT64_C( 55340232221129), // 2^64 * 0.000003
	UINT64_C( 73786976294838), // 2^64 * 0.000004
	UINT64_C( 92233720368548), // 2^64 * 0.000005
	UINT64_C(110680464442257), // 2^64 * 0.000006
	UINT64_C(129127208515967), // 2^64 * 0.000007
	UINT64_C(147573952589676), // 2^64 * 0.000008
	UINT64_C(166020696663386), // 2^64 * 0.000009
	UINT64_C( 1844674407371), // 2^64 * 0.0000001
	UINT64_C( 3689348814742), // 2^64 * 0.0000002
	UINT64_C( 5534023222113), // 2^64 * 0.0000003
	UINT64_C( 7378697629484), // 2^64 * 0.0000004
	UINT64_C( 9223372036855), // 2^64 * 0.0000005
	UINT64_C(11068046444226), // 2^64 * 0.0000006
	UINT64_C(12912720851597), // 2^64 * 0.0000007
	UINT64_C(14757395258968), // 2^64 * 0.0000008
	UINT64_C(16602069666339), // 2^64 * 0.0000009
	UINT64_C( 184467440737), // 2^64 * 0.00000001
	UINT64_C( 368934881474), // 2^64 * 0.00000002
	UINT64_C( 553402322211), // 2^64 * 0.00000003
	UINT64_C( 737869762948), // 2^64 * 0.00000004
	UINT64_C( 922337203685), // 2^64 * 0.00000005
	UINT64_C(1106804644423), // 2^64 * 0.00000006
	UINT64_C(1291272085160), // 2^64 * 0.00000007
	UINT64_C(1475739525897), // 2^64 * 0.00000008
	UINT64_C(1660206966634), // 2^64 * 0.00000009
	UINT64_C( 18446744074), // 2^64 * 0.000000001
	UINT64_C( 36893488147), // 2^64 * 0.000000002
	UINT64_C( 55340232221), // 2^64 * 0.000000003
	UINT64_C( 73786976295), // 2^64 * 0.000000004
	UINT64_C( 92233720369), // 2^64 * 0.000000005
	UINT64_C(110680464442), // 2^64 * 0.000000006
	UINT64_C(129127208516), // 2^64 * 0.000000007
	UINT64_C(147573952590), // 2^64 * 0.000000008
	UINT64_C(166020696663), // 2^64 * 0.000000009
	UINT64_C( 1844674407), // 2^64 * 0.0000000001
	UINT64_C( 3689348815), // 2^64 * 0.0000000002
	UINT64_C( 5534023222), // 2^64 * 0.0000000003
	UINT64_C( 7378697629), // 2^64 * 0.0000000004
	UINT64_C( 9223372037), // 2^64 * 0.0000000005
	UINT64_C(11068046444), // 2^64 * 0.0000000006
	UINT64_C(12912720852), // 2^64 * 0.0000000007
	UINT64_C(14757395259), // 2^64 * 0.0000000008
	UINT64_C(16602069666), // 2^64 * 0.0000000009
	UINT64_C( 184467441), // 2^64 * 0.00000000001
	UINT64_C( 368934881), // 2^64 * 0.00000000002
	UINT64_C( 553402322), // 2^64 * 0.00000000003
	UINT64_C( 737869763), // 2^64 * 0.00000000004
	UINT64_C( 922337204), // 2^64 * 0.00000000005
	UINT64_C(1106804644), // 2^64 * 0.00000000006
	UINT64_C(1291272085), // 2^64 * 0.00000000007
	UINT64_C(1475739526), // 2^64 * 0.00000000008
	UINT64_C(1660206967), // 2^64 * 0.00000000009
	UINT64_C( 18446744), // 2^64 * 0.000000000001
	UINT64_C( 36893488), // 2^64 * 0.000000000002
	UINT64_C( 55340232), // 2^64 * 0.000000000003
	UINT64_C( 73786976), // 2^64 * 0.000000000004
	UINT64_C( 92233720), // 2^64 * 0.000000000005
	UINT64_C(110680464), // 2^64 * 0.000000000006
	UINT64_C(129127209), // 2^64 * 0.000000000007
	UINT64_C(147573953), // 2^64 * 0.000000000008
	UINT64_C(166020697), // 2^64 * 0.000000000009
	UINT64_C( 1844674), // 2^64 * 0.0000000000001
	UINT64_C( 3689349), // 2^64 * 0.0000000000002
	UINT64_C( 5534023), // 2^64 * 0.0000000000003
	UINT64_C( 7378698), // 2^64 * 0.0000000000004
	UINT64_C( 9223372), // 2^64 * 0.0000000000005
	UINT64_C(11068046), // 2^64 * 0.0000000000006
	UINT64_C(12912721), // 2^64 * 0.0000000000007
	UINT64_C(14757395), // 2^64 * 0.0000000000008
	UINT64_C(16602070), // 2^64 * 0.0000000000009
	UINT64_C( 184467), // 2^64 * 0.00000000000001
	UINT64_C( 368935), // 2^64 * 0.00000000000002
	UINT64_C( 553402), // 2^64 * 0.00000000000003
	UINT64_C( 737870), // 2^64 * 0.00000000000004
	UINT64_C( 922337), // 2^64 * 0.00000000000005
	UINT64_C(1106805), // 2^64 * 0.00000000000006
	UINT64_C(1291272), // 2^64 * 0.00000000000007
	UINT64_C(1475740), // 2^64 * 0.00000000000008
	UINT64_C(1660207), // 2^64 * 0.00000000000009
	UINT64_C( 18447), // 2^64 * 0.000000000000001
	UINT64_C( 36893), // 2^64 * 0.000000000000002
	UINT64_C( 55340), // 2^64 * 0.000000000000003
	UINT64_C( 73787), // 2^64 * 0.000000000000004
	UINT64_C( 92234), // 2^64 * 0.000000000000005
	UINT64_C(110680), // 2^64 * 0.000000000000006
	UINT64_C(129127), // 2^64 * 0.000000000000007
	UINT64_C(147574), // 2^64 * 0.000000000000008
	UINT64_C(166021), // 2^64 * 0.000000000000009
	UINT64_C( 1845), // 2^64 * 0.0000000000000001
	UINT64_C( 3689), // 2^64 * 0.0000000000000002
	UINT64_C( 5534), // 2^64 * 0.0000000000000003
	UINT64_C( 7379), // 2^64 * 0.0000000000000004
	UINT64_C( 9223), // 2^64 * 0.0000000000000005
	UINT64_C(11068), // 2^64 * 0.0000000000000006
	UINT64_C(12913), // 2^64 * 0.0000000000000007
	UINT64_C(14757), // 2^64 * 0.0000000000000008
	UINT64_C(16602), // 2^64 * 0.0000000000000009
	UINT64_C( 184), // 2^64 * 0.00000000000000001
	UINT64_C( 369), // 2^64 * 0.00000000000000002
	UINT64_C( 553), // 2^64 * 0.00000000000000003
	UINT64_C( 738), // 2^64 * 0.00000000000000004
	UINT64_C( 922), // 2^64 * 0.00000000000000005
	UINT64_C(1107), // 2^64 * 0.00000000000000006
	UINT64_C(1291), // 2^64 * 0.00000000000000007
	UINT64_C(1476), // 2^64 * 0.00000000000000008
	UINT64_C(1660), // 2^64 * 0.00000000000000009
	UINT64_C( 18), // 2^64 * 0.000000000000000001
	UINT64_C( 37), // 2^64 * 0.000000000000000002
	UINT64_C( 55), // 2^64 * 0.000000000000000003
	UINT64_C( 74), // 2^64 * 0.000000000000000004
	UINT64_C( 92), // 2^64 * 0.000000000000000005
	UINT64_C(111), // 2^64 * 0.000000000000000006
	UINT64_C(129), // 2^64 * 0.000000000000000007
	UINT64_C(148), // 2^64 * 0.000000000000000008
	UINT64_C(166), // 2^64 * 0.000000000000000009
	UINT64_C( 2), // 2^64 * 0.0000000000000000001
	UINT64_C( 4), // 2^64 * 0.0000000000000000002
	UINT64_C( 6), // 2^64 * 0.0000000000000000003
	UINT64_C( 7), // 2^64 * 0.0000000000000000004
	UINT64_C( 9), // 2^64 * 0.0000000000000000005
	UINT64_C(11), // 2^64 * 0.0000000000000000006
	UINT64_C(13), // 2^64 * 0.0000000000000000007
	UINT64_C(15), // 2^64 * 0.0000000000000000008
	UINT64_C(17), // 2^64 * 0.0000000000000000009
};
// Index 0 returns 0.1 in the 64-bit fraction system
// Index 1 represents 0.01, et cetera
static const uint64_t getDecimalFraction64(int decimalPosition, int digit) {
	if (decimalPosition < 0 || decimalPosition >= maxDecimals || digit < 1 || digit > 9) {
		return 0;
	} else {
		return decimalFractions64[(decimalPosition * 9) + (digit - 1)];
	}
}

FixedPoint::FixedPoint() : mantissa(0) {}

FixedPoint::FixedPoint(int64_t newMantissa) {
	clampForInt32(newMantissa);
	this->mantissa = newMantissa;
}

FixedPoint FixedPoint::fromWhole(int64_t wholeInteger) {
	clampForSaturatedWhole(wholeInteger);
	return FixedPoint(wholeInteger * 65536); // Does this need to saturate again?
}

FixedPoint FixedPoint::fromMantissa(int64_t mantissa) {
	return FixedPoint(mantissa);
}

FixedPoint FixedPoint::fromText(const ReadableString& text) {
	ReadableString content = string_removeOuterWhiteSpace(text);
	bool isSigned = string_findFirst(content, U'-') > -1; // Should also be last
	int decimal = string_findFirst(content, U'.');
	int colon = string_findFirst(content, U':');
	int64_t result = 0;
	if (decimal > -1 && colon == -1) {
		// Floating-point decimal
		// TODO: Give warnings for incorrect whole integers
		int64_t wholeInteger = string_toInteger(string_before(content, decimal));
		ReadableString decimals = string_after(content, decimal);
		uint64_t fraction = 0; // Extra high precision for accumulation
		for (int i = 0; i < string_length(decimals); i++) {
			DsrChar digit = decimals[i];
			if (digit >= U'1' && digit <= U'9') {
				fraction += getDecimalFraction64(i, digit - U'0');
			} // else if (digit != U'0') // TODO: Give warnings for any non-digit characters.
		}
		// Truncate the fraction down to 32-bits before safely rounding to closest 16-bit fraction
		int64_t signedFraction = ((fraction >> 32) + 32768) >> 16; // Convert to closest 16-bit fraction
		if (isSigned) { signedFraction = -signedFraction; }
		result = (wholeInteger * 65536) + signedFraction; // Does this need to saturate again?
	} else if (decimal == -1 && colon > -1) {
		// Whole integer and 16-bit fraction
		// TODO: Give warnings for incorrect integers
		int64_t wholeInteger = string_toInteger(string_before(content, colon));
		int64_t fraction = string_toInteger(string_after(content, colon));
		clampForSaturatedWhole(wholeInteger);
		if (isSigned) { fraction = -fraction; }
		result = (wholeInteger * 65536) + fraction;
	} else if (decimal == -1 && colon == -1) {
		// Whole
		int64_t wholeInteger = string_toInteger(content);
		clampForSaturatedWhole(wholeInteger);
		result = wholeInteger * 65536; // Does this need to saturate again?
	} // TODO: Give a warning if both . and : is used!
	return FixedPoint(result);
}

FixedPoint FixedPoint::zero() {
	return FixedPoint(0);
}
FixedPoint FixedPoint::epsilon() {
	return FixedPoint(1);
}
FixedPoint FixedPoint::half() {
	return FixedPoint(32768);
}
FixedPoint FixedPoint::one() {
	return FixedPoint(65536);
}

int32_t dsr::fixedPoint_round(const FixedPoint& value) {
	int64_t mantissa = value.getMantissa();
	int32_t offset = mantissa >= 0 ? 32768 : -32768;
	return (mantissa + offset) / 65536;
}

double dsr::fixedPoint_approximate(const FixedPoint& value) {
	return ((double)value.getMantissa()) * (1.0 / 65536.0);
}

String& dsr::string_toStreamIndented(String& target, const FixedPoint& value, const ReadableString& indentation) {
	// TODO: Make own fixed-point serialization which cannot resort to scientific notation
	string_append(target, indentation, fixedPoint_approximate(value));
	return target;
}

FixedPoint dsr::fixedPoint_min(const FixedPoint &left, const FixedPoint &right) {
	int64_t result = left.getMantissa();
	int64_t other = right.getMantissa();
	if (other < result) result = other;
	return FixedPoint(result);
}

FixedPoint dsr::fixedPoint_max(const FixedPoint &left, const FixedPoint &right) {
	int64_t result = left.getMantissa();
	int64_t other = right.getMantissa();
	if (other > result) result = other;
	return FixedPoint(result);
}

FixedPoint dsr::fixedPoint_divide(const FixedPoint &left, const FixedPoint &right) {
	int64_t mantissa = 0;
	if (right.getMantissa() == 0) {
		if (left.getMantissa() > 0) {
			mantissa = 2147483647; // Saturate from positive infinity
		} else if (left.getMantissa() < 0) {
			mantissa = -2147483648; // Saturate from negative infinity
		}
	} else {
		mantissa = (left.getMantissa() * 65536) / right.getMantissa();
	}
	return FixedPoint(mantissa);
}
FixedPoint dsr::fixedPoint_divide(const FixedPoint &left, int64_t right) {
	int64_t mantissa = 0;
	if (right == 0) {
		if (left.getMantissa() > 0) {
			mantissa = 2147483647; // Saturate from positive infinity
		} else if (left.getMantissa() < 0) {
			mantissa = -2147483648; // Saturate from negative infinity
		}
	} else {
		mantissa = left.getMantissa() / right;
	}
	return FixedPoint(mantissa);
}

// 48-bit to 24-bit unsigned integer square root.
//   Returns the root of square rounded down.
static uint64_t integer_squareRoot_U48(uint64_t square) {
	// Even thou a double is used, the C++ standard guarantees exact results.
	// Source: https://en.cppreference.com/w/cpp/numeric/math/sqrt
	//   "std::sqrt is required by the IEEE standard to be exact.
	//    The only other operations required to be exact are the arithmetic
	//    operators and the function std::fma. After rounding to the return
	//    type (using default rounding mode), the result of std::sqrt is
	//    indistinguishable from the infinitely precise result.
	//    In other words, the error is less than 0.5 ulp."
	return (uint64_t)(std::sqrt((double)square));
}

FixedPoint dsr::fixedPoint_squareRoot(const FixedPoint& value) {
	int64_t mantissa = value.getMantissa();
	if (mantissa <= 0) {
		// The real part of 0 + i * sqrt(value) is always zero
		return FixedPoint(0);
	} else {
		return FixedPoint(integer_squareRoot_U48(((uint64_t)mantissa) << 16));
	}
}
