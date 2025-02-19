
#include "../testTools.h"
#include "../../DFPSR/collection/Field.h"

START_TEST(Field)
	{
		// Allocate 3 x 2 integers, initialized to 123 in each element.
		Field<int> a = Field<int>(3, 2, 123);
		ASSERT_EQUAL(a.width(), 3);
		ASSERT_EQUAL(a.height(), 2);
		ASSERT_EQUAL(string_combine(a),
		  U"{\n"
		  U"	{\n"
		  U"		123,\n"
		  U"		123,\n"
		  U"		123\n"
		  U"	},\n"
		  U"	{\n"
		  U"		123,\n"
		  U"		123,\n"
		  U"		123\n"
		  U"	}\n"
		  U"}"
		);
		// Skip writing outside.
		a.write_ignore(-1, 0, 55555);
		a.write_ignore(3, 1, 88);		
		a.write_ignore(0, -1, 999);
		a.write_ignore(2, 2, 12345);
		// Write inside.
		a.write_ignore(0, 0, 11);
		a.write_ignore(1, 0, 21);
		a.write_ignore(0, 1, 12);
		// Copy to b.
		Field<int> b = a;
		// Write inside.
		a.write_ignore(2, 0, 31);
		a.write_ignore(1, 1, 22);
		a.write_ignore(2, 1, 32);
		ASSERT_EQUAL(string_combine(a),
		  U"{\n"
		  U"	{\n"
		  U"		11,\n"
		  U"		21,\n"
		  U"		31\n"
		  U"	},\n"
		  U"	{\n"
		  U"		12,\n"
		  U"		22,\n"
		  U"		32\n"
		  U"	}\n"
		  U"}"
		);
		ASSERT_EQUAL(string_combine(b),
		  U"{\n"
		  U"	{\n"
		  U"		11,\n"
		  U"		21,\n"
		  U"		123\n"
		  U"	},\n"
		  U"	{\n"
		  U"		12,\n"
		  U"		123,\n"
		  U"		123\n"
		  U"	}\n"
		  U"}"
		);
		// Read with border.
		ASSERT_EQUAL(a.read_border(-2, -2,  8),  8); // Outside
		ASSERT_EQUAL(a.read_border(-1, -2, -1), -1); // Outside
		ASSERT_EQUAL(a.read_border(-1, -1, -1), -1); // Outside
		ASSERT_EQUAL(a.read_border( 0, -1, -1), -1); // Outside
		ASSERT_EQUAL(a.read_border( 0,  0, -2), 11); // Inside
		ASSERT_EQUAL(a.read_border( 1,  0, -1), 21); // Inside
		ASSERT_EQUAL(a.read_border( 1,  1, 55), 22); // Inside
		ASSERT_EQUAL(a.read_border( 2,  1, -1), 32); // Inside
		ASSERT_EQUAL(a.read_border( 2,  2, 12), 12); // Outside
		ASSERT_EQUAL(a.read_border( 3,  2, -1), -1); // Outside
		ASSERT_EQUAL(a.read_border( 3,  3, 13), 13); // Outside
		ASSERT_EQUAL(a.read_border( 4,  3, -1), -1); // Outside
		// Read with clamping.
		ASSERT_EQUAL(a.read_clamp(-2, -2), 11); // Outside
		ASSERT_EQUAL(a.read_clamp(-1, -2), 11); // Outside
		ASSERT_EQUAL(a.read_clamp(-1, -1), 11); // Outside
		ASSERT_EQUAL(a.read_clamp( 0, -1), 11); // Outside
		ASSERT_EQUAL(a.read_clamp( 0,  0), 11); // Outside
		ASSERT_EQUAL(a.read_clamp( 1,  0), 21); // Inside
		ASSERT_EQUAL(a.read_clamp( 1,  1), 22); // Inside
		ASSERT_EQUAL(a.read_clamp( 2,  1), 32); // Inside
		ASSERT_EQUAL(a.read_clamp( 2,  2), 32); // Outside
		ASSERT_EQUAL(a.read_clamp(-1,  2), 12); // Outside
		ASSERT_EQUAL(a.read_clamp( 3,  3), 32); // Outside
		ASSERT_EQUAL(a.read_clamp( 4, -1), 31); // Outside
		// Assign b to a and check that they went from not equal to equal.
		ASSERT_NOT_EQUAL(a, b);
		a = b;
		ASSERT_EQUAL(a, b);
	}
END_TEST
