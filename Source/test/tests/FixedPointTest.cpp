
#include "../testTools.h"
#include "../../DFPSR/math/FixedPoint.h"

#define MANTISSA(VALUE) FixedPoint(VALUE)
#define WHOLE(VALUE) FixedPoint::fromWhole(VALUE)

START_TEST(FixedPoint)
	// Comparison
	ASSERT_EQUAL(MANTISSA(-43), MANTISSA(-43));
	ASSERT_EQUAL(MANTISSA(0), MANTISSA(0));
	ASSERT_EQUAL(MANTISSA(2644), MANTISSA(2644));
	ASSERT_EQUAL(WHOLE(-360), WHOLE(-360));
	ASSERT_EQUAL(WHOLE(0), WHOLE(0));
	ASSERT_EQUAL(WHOLE(645), WHOLE(645));
	ASSERT_EQUAL(WHOLE(645), 645);
	ASSERT_EQUAL(645, WHOLE(645));
	// Addition
	ASSERT_EQUAL(WHOLE(1030) + WHOLE(204), 1234);
	ASSERT_EQUAL(WHOLE(1030) + 204, 1234);
	ASSERT_EQUAL(1030 + WHOLE(204), 1234);
	// Subtraction
	ASSERT_EQUAL(WHOLE(355) - WHOLE(55), 300);
	ASSERT_EQUAL(WHOLE(355) - 55, 300);
	ASSERT_EQUAL(355 - WHOLE(55), 300);
	// Multiplication
	ASSERT_EQUAL(WHOLE(25) * WHOLE(4), 100);
	ASSERT_EQUAL(WHOLE(25) * 4, 100);
	ASSERT_EQUAL(25 * WHOLE(4), 100);
	ASSERT_EQUAL(WHOLE(10) * WHOLE(2), 20);
	ASSERT_EQUAL(WHOLE(-10) * WHOLE(-2), 20);
	ASSERT_EQUAL(WHOLE(-10) * WHOLE(2), -20);
	ASSERT_EQUAL(WHOLE(10) * WHOLE(-2), -20);
	// Division
	ASSERT_EQUAL(WHOLE(35) / WHOLE(5), 7);
	ASSERT_EQUAL(WHOLE(35) / 5, 7);
	ASSERT_EQUAL(35 / WHOLE(5), 7);
	ASSERT_EQUAL(WHOLE(2000) / WHOLE(20), 100);
	ASSERT_EQUAL(WHOLE(-2000) / WHOLE(-20), 100);
	ASSERT_EQUAL(WHOLE(-2000) / WHOLE(20), -100);
	ASSERT_EQUAL(WHOLE(2000) / WHOLE(-20), -100);
	ASSERT_EQUAL(WHOLE(0) / WHOLE(0), 0);
	ASSERT_EQUAL(0 / WHOLE(0), 0);
	ASSERT_EQUAL(WHOLE(0) / 0, 0);
	ASSERT_EQUAL(WHOLE(1) / WHOLE(0), MANTISSA(2147483647));
	ASSERT_EQUAL(1 / WHOLE(0), MANTISSA(2147483647));
	ASSERT_EQUAL(WHOLE(1) / 0, MANTISSA(2147483647));
	ASSERT_EQUAL(WHOLE(-1) / WHOLE(0), MANTISSA(-2147483648));
	ASSERT_EQUAL(-1 / WHOLE(0), MANTISSA(-2147483648));
	ASSERT_EQUAL(WHOLE(-1) / 0, MANTISSA(-2147483648));
	// Parsing decimals from text should round to closest
	ASSERT_EQUAL(FixedPoint::fromText(U"1.000000000000000001"), WHOLE(1));
	ASSERT_EQUAL(FixedPoint::fromText(U"-1.000000000000000001"), WHOLE(-1));
	ASSERT_EQUAL(FixedPoint::fromText(U"0.000000000000000001"), WHOLE(0));
	ASSERT_EQUAL(FixedPoint::fromText(U"-0.000000000000000001"), WHOLE(0));
	ASSERT_EQUAL(FixedPoint::fromText(U"0.999999999999999999"), WHOLE(1));
	ASSERT_EQUAL(FixedPoint::fromText(U"-0.999999999999999999"), WHOLE(-1));
	// Half values should be bit-exact
	ASSERT_EQUAL(FixedPoint::fromText(U"0.5"), MANTISSA(32768));
	ASSERT_EQUAL(FixedPoint::fromText(U"-0.5"), MANTISSA(-32768));
	ASSERT_EQUAL(FixedPoint::fromText(U"0:32768"), MANTISSA(32768));
	ASSERT_EQUAL(FixedPoint::fromText(U"-0:32768"), MANTISSA(-32768));
	ASSERT_EQUAL(FixedPoint::fromText(U"1.5"), MANTISSA(98304));
	ASSERT_EQUAL(FixedPoint::fromText(U"-1.5"), MANTISSA(-98304));
	// Allow outside space and extra zeroes
	ASSERT_EQUAL(FixedPoint::fromText(U"	001:000"), WHOLE(1));
	ASSERT_EQUAL(FixedPoint::fromText(U"000503.000 "), WHOLE(503));
	// Whole values should remain whole
	int errorCount_whole = 0;
	for (int i = -32768; i < 32767; i++) {
		String textValue = string_combine(i);
		if (FixedPoint::fromText(textValue) != WHOLE(i)) { errorCount_whole++; }
	}
	ASSERT_EQUAL(errorCount_whole, 0);
	int errorCount_decimal = 0;
	for (int i = -32768; i < 32767; i++) {
		String textValue = string_combine(i, U".0");
		if (FixedPoint::fromText(textValue) != WHOLE(i)) { errorCount_decimal++; }
	}
	ASSERT_EQUAL(errorCount_decimal, 0);
	int errorCount_remainder = 0;
	for (int i = -32768; i < 32767; i++) {
		String textValue = string_combine(i, U":0");
		if (FixedPoint::fromText(textValue) != WHOLE(i)) { errorCount_remainder++; }
	}
	ASSERT_EQUAL(errorCount_remainder, 0);
	// Saturating should use the whole range including fractions
	ASSERT_EQUAL(FixedPoint::fromText(U"-453764573.34576012934264576354"), MANTISSA(-2147483648));
	ASSERT_EQUAL(FixedPoint::fromText(U"207284572931.60298753343645345"), MANTISSA(2147483647));
	// Rounding to whole integers
	ASSERT_EQUAL(fixedPoint_round(FixedPoint::fromText(U"1528.34")), 1528);
	ASSERT_EQUAL(fixedPoint_round(FixedPoint::fromText(U"-864.51")), -865);
	ASSERT_EQUAL(fixedPoint_round(FixedPoint::fromText(U"0.49")), 0);
	ASSERT_EQUAL(fixedPoint_round(FixedPoint::fromText(U"0.5")), 1);
	ASSERT_EQUAL(fixedPoint_round(FixedPoint::fromText(U"0.51")), 1);
	ASSERT_EQUAL(fixedPoint_round(FixedPoint::fromText(U"-0.49")), 0);
	ASSERT_EQUAL(fixedPoint_round(FixedPoint::fromText(U"-0.5")), -1);
	ASSERT_EQUAL(fixedPoint_round(FixedPoint::fromText(U"-0.51")), -1);
	// Square roots
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(-1000)), WHOLE(0));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(-1)), WHOLE(0));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(0)), WHOLE(0));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(1)), WHOLE(1));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(4)), WHOLE(2));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(9)), WHOLE(3));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(16)), WHOLE(4));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(25)), WHOLE(5));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(36)), WHOLE(6));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(49)), WHOLE(7));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(64)), WHOLE(8));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(81)), WHOLE(9));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(100)), WHOLE(10));
	ASSERT_EQUAL(fixedPoint_squareRoot(WHOLE(10000)), WHOLE(100));
END_TEST
