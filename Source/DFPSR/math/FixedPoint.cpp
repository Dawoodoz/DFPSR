
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
	 1844674407370955162ull, // 2^64 * 0.1
	 3689348814741910323ull, // 2^64 * 0.2
	 5534023222112865485ull, // 2^64 * 0.3
	 7378697629483820646ull, // 2^64 * 0.4
	 9223372036854775808ull, // 2^64 * 0.5
	11068046444225730970ull, // 2^64 * 0.6
	12912720851596686131ull, // 2^64 * 0.7
	14757395258967641293ull, // 2^64 * 0.8
	16602069666338596454ull, // 2^64 * 0.9
	 184467440737095516ull, // 2^64 * 0.01
	 368934881474191032ull, // 2^64 * 0.02
	 553402322211286548ull, // 2^64 * 0.03
	 737869762948382065ull, // 2^64 * 0.04
	 922337203685477581ull, // 2^64 * 0.05
	1106804644422573097ull, // 2^64 * 0.06
	1291272085159668613ull, // 2^64 * 0.07
	1475739525896764129ull, // 2^64 * 0.08
	1660206966633859645ull, // 2^64 * 0.09
	 18446744073709552ull, // 2^64 * 0.001
	 36893488147419103ull, // 2^64 * 0.002
	 55340232221128655ull, // 2^64 * 0.003
	 73786976294838206ull, // 2^64 * 0.004
	 92233720368547758ull, // 2^64 * 0.005
	110680464442257310ull, // 2^64 * 0.006
	129127208515966861ull, // 2^64 * 0.007
	147573952589676413ull, // 2^64 * 0.008
	166020696663385965ull, // 2^64 * 0.009
	 1844674407370955ull, // 2^64 * 0.0001
	 3689348814741910ull, // 2^64 * 0.0002
	 5534023222112865ull, // 2^64 * 0.0003
	 7378697629483821ull, // 2^64 * 0.0004
	 9223372036854776ull, // 2^64 * 0.0005
	11068046444225731ull, // 2^64 * 0.0006
	12912720851596686ull, // 2^64 * 0.0007
	14757395258967641ull, // 2^64 * 0.0008
	16602069666338596ull, // 2^64 * 0.0009
	 184467440737096ull, // 2^64 * 0.00001
	 368934881474191ull, // 2^64 * 0.00002
	 553402322211287ull, // 2^64 * 0.00003
	 737869762948382ull, // 2^64 * 0.00004
	 922337203685478ull, // 2^64 * 0.00005
	1106804644422573ull, // 2^64 * 0.00006
	1291272085159669ull, // 2^64 * 0.00007
	1475739525896764ull, // 2^64 * 0.00008
	1660206966633860ull, // 2^64 * 0.00009
	 18446744073710ull, // 2^64 * 0.000001
	 36893488147419ull, // 2^64 * 0.000002
	 55340232221129ull, // 2^64 * 0.000003
	 73786976294838ull, // 2^64 * 0.000004
	 92233720368548ull, // 2^64 * 0.000005
	110680464442257ull, // 2^64 * 0.000006
	129127208515967ull, // 2^64 * 0.000007
	147573952589676ull, // 2^64 * 0.000008
	166020696663386ull, // 2^64 * 0.000009
	 1844674407371ull, // 2^64 * 0.0000001
	 3689348814742ull, // 2^64 * 0.0000002
	 5534023222113ull, // 2^64 * 0.0000003
	 7378697629484ull, // 2^64 * 0.0000004
	 9223372036855ull, // 2^64 * 0.0000005
	11068046444226ull, // 2^64 * 0.0000006
	12912720851597ull, // 2^64 * 0.0000007
	14757395258968ull, // 2^64 * 0.0000008
	16602069666339ull, // 2^64 * 0.0000009
	 184467440737ull, // 2^64 * 0.00000001
	 368934881474ull, // 2^64 * 0.00000002
	 553402322211ull, // 2^64 * 0.00000003
	 737869762948ull, // 2^64 * 0.00000004
	 922337203685ull, // 2^64 * 0.00000005
	1106804644423ull, // 2^64 * 0.00000006
	1291272085160ull, // 2^64 * 0.00000007
	1475739525897ull, // 2^64 * 0.00000008
	1660206966634ull, // 2^64 * 0.00000009
	 18446744074ull, // 2^64 * 0.000000001
	 36893488147ull, // 2^64 * 0.000000002
	 55340232221ull, // 2^64 * 0.000000003
	 73786976295ull, // 2^64 * 0.000000004
	 92233720369ull, // 2^64 * 0.000000005
	110680464442ull, // 2^64 * 0.000000006
	129127208516ull, // 2^64 * 0.000000007
	147573952590ull, // 2^64 * 0.000000008
	166020696663ull, // 2^64 * 0.000000009
	 1844674407ull, // 2^64 * 0.0000000001
	 3689348815ull, // 2^64 * 0.0000000002
	 5534023222ull, // 2^64 * 0.0000000003
	 7378697629ull, // 2^64 * 0.0000000004
	 9223372037ull, // 2^64 * 0.0000000005
	11068046444ull, // 2^64 * 0.0000000006
	12912720852ull, // 2^64 * 0.0000000007
	14757395259ull, // 2^64 * 0.0000000008
	16602069666ull, // 2^64 * 0.0000000009
	 184467441ull, // 2^64 * 0.00000000001
	 368934881ull, // 2^64 * 0.00000000002
	 553402322ull, // 2^64 * 0.00000000003
	 737869763ull, // 2^64 * 0.00000000004
	 922337204ull, // 2^64 * 0.00000000005
	1106804644ull, // 2^64 * 0.00000000006
	1291272085ull, // 2^64 * 0.00000000007
	1475739526ull, // 2^64 * 0.00000000008
	1660206967ull, // 2^64 * 0.00000000009
	 18446744ull, // 2^64 * 0.000000000001
	 36893488ull, // 2^64 * 0.000000000002
	 55340232ull, // 2^64 * 0.000000000003
	 73786976ull, // 2^64 * 0.000000000004
	 92233720ull, // 2^64 * 0.000000000005
	110680464ull, // 2^64 * 0.000000000006
	129127209ull, // 2^64 * 0.000000000007
	147573953ull, // 2^64 * 0.000000000008
	166020697ull, // 2^64 * 0.000000000009
	 1844674ull, // 2^64 * 0.0000000000001
	 3689349ull, // 2^64 * 0.0000000000002
	 5534023ull, // 2^64 * 0.0000000000003
	 7378698ull, // 2^64 * 0.0000000000004
	 9223372ull, // 2^64 * 0.0000000000005
	11068046ull, // 2^64 * 0.0000000000006
	12912721ull, // 2^64 * 0.0000000000007
	14757395ull, // 2^64 * 0.0000000000008
	16602070ull, // 2^64 * 0.0000000000009
	 184467ull, // 2^64 * 0.00000000000001
	 368935ull, // 2^64 * 0.00000000000002
	 553402ull, // 2^64 * 0.00000000000003
	 737870ull, // 2^64 * 0.00000000000004
	 922337ull, // 2^64 * 0.00000000000005
	1106805ull, // 2^64 * 0.00000000000006
	1291272ull, // 2^64 * 0.00000000000007
	1475740ull, // 2^64 * 0.00000000000008
	1660207ull, // 2^64 * 0.00000000000009
	 18447ull, // 2^64 * 0.000000000000001
	 36893ull, // 2^64 * 0.000000000000002
	 55340ull, // 2^64 * 0.000000000000003
	 73787ull, // 2^64 * 0.000000000000004
	 92234ull, // 2^64 * 0.000000000000005
	110680ull, // 2^64 * 0.000000000000006
	129127ull, // 2^64 * 0.000000000000007
	147574ull, // 2^64 * 0.000000000000008
	166021ull, // 2^64 * 0.000000000000009
	 1845ull, // 2^64 * 0.0000000000000001
	 3689ull, // 2^64 * 0.0000000000000002
	 5534ull, // 2^64 * 0.0000000000000003
	 7379ull, // 2^64 * 0.0000000000000004
	 9223ull, // 2^64 * 0.0000000000000005
	11068ull, // 2^64 * 0.0000000000000006
	12913ull, // 2^64 * 0.0000000000000007
	14757ull, // 2^64 * 0.0000000000000008
	16602ull, // 2^64 * 0.0000000000000009
	 184ull, // 2^64 * 0.00000000000000001
	 369ull, // 2^64 * 0.00000000000000002
	 553ull, // 2^64 * 0.00000000000000003
	 738ull, // 2^64 * 0.00000000000000004
	 922ull, // 2^64 * 0.00000000000000005
	1107ull, // 2^64 * 0.00000000000000006
	1291ull, // 2^64 * 0.00000000000000007
	1476ull, // 2^64 * 0.00000000000000008
	1660ull, // 2^64 * 0.00000000000000009
	 18ull, // 2^64 * 0.000000000000000001
	 37ull, // 2^64 * 0.000000000000000002
	 55ull, // 2^64 * 0.000000000000000003
	 74ull, // 2^64 * 0.000000000000000004
	 92ull, // 2^64 * 0.000000000000000005
	111ull, // 2^64 * 0.000000000000000006
	129ull, // 2^64 * 0.000000000000000007
	148ull, // 2^64 * 0.000000000000000008
	166ull, // 2^64 * 0.000000000000000009
	 2ull, // 2^64 * 0.0000000000000000001
	 4ull, // 2^64 * 0.0000000000000000002
	 6ull, // 2^64 * 0.0000000000000000003
	 7ull, // 2^64 * 0.0000000000000000004
	 9ull, // 2^64 * 0.0000000000000000005
	11ull, // 2^64 * 0.0000000000000000006
	13ull, // 2^64 * 0.0000000000000000007
	15ull, // 2^64 * 0.0000000000000000008
	17ull, // 2^64 * 0.0000000000000000009
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
	bool isSigned = content.findFirst(U'-') > -1; // Should also be last
	int decimal = content.findFirst(U'.');
	int colon = content.findFirst(U':');
	int64_t result = 0;
	if (decimal > -1 && colon == -1) {
		// Floating-point decimal
		// TODO: Give warnings for incorrect whole integers
		int64_t wholeInteger = string_parseInteger(content.before(decimal));
		ReadableString decimals = content.after(decimal);
		uint64_t fraction = 0; // Extra high precision for accumulation
		for (int i = 0; i < decimals.length(); i++) {
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
		int64_t wholeInteger = string_parseInteger(content.before(colon));
		int64_t fraction = string_parseInteger(content.after(colon));
		clampForSaturatedWhole(wholeInteger);
		if (isSigned) { fraction = -fraction; }
		result = (wholeInteger * 65536) + fraction;
	} else if (decimal == -1 && colon == -1) {
		// Whole
		int64_t wholeInteger = string_parseInteger(content);
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
