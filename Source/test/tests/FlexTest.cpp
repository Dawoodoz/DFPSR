
#include "../testTools.h"
#include "../../DFPSR/gui/FlexRegion.h"

START_TEST(Flex)
	// Comparisons
	ASSERT_EQUAL(FlexValue(-1846, 23), FlexValue(0, 23)); // Limited to 0%
	ASSERT_EQUAL(FlexValue(346, -54), FlexValue(100, -54)); // Limited to 100%
	ASSERT_NOT_EQUAL(FlexValue(67, 34), FlexValue(57, 34));
	ASSERT_NOT_EQUAL(FlexValue(14, 24), FlexValue(14, 84));
	FlexValue a;
	a.assignValue(U"67%+34", U"");
	ASSERT_EQUAL(a, FlexValue(67, 34));
	ASSERT_EQUAL(FlexValue(67, 34).toString(), U"67%+34");
END_TEST

