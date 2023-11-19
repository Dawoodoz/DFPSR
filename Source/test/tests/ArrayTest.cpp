
#include "../testTools.h"

START_TEST(Array)
	{
		Array<int> a = Array<int>(4, 123);
		a[1] = 85;
		a[3] = -100;
		ASSERT_EQUAL(a.length(), 4);
		ASSERT_EQUAL(a[0], 123);
		ASSERT_EQUAL(a[1], 85);
		ASSERT_EQUAL(a[2], 123);
		ASSERT_EQUAL(a[3], -100);
		ASSERT_EQUAL(string_combine(a),
		  U"{\n"
		  U"	123,\n"
		  U"	85,\n"
		  U"	123,\n"
		  U"	-100\n"
		  U"}"
		);
		// An initial assignment uses the copy constructor, because there is no pre-existing data in b.
		Array<int> b = a;
		b[0] = 200;
		b[2] = 100000;
		// The b array has changed...
		ASSERT_EQUAL(string_combine(b),
		  U"{\n"
		  U"	200,\n"
		  U"	85,\n"
		  U"	100000,\n"
		  U"	-100\n"
		  U"}"
		);
		// ...but a remains the same, because the data was cloned when assigning.
		ASSERT_EQUAL(string_combine(a),
		  U"{\n"
		  U"	123,\n"
		  U"	85,\n"
		  U"	123,\n"
		  U"	-100\n"
		  U"}"
		);
		// They are not equal
		ASSERT_NOT_EQUAL(a, b);
		// Assigning from copy construction is optimized into an assignment operation, because b already exists.
		a = Array<int>(b);
		// Now they are equal
		ASSERT_EQUAL(a, b);
		// Create another length
		Array<int> c = Array<int>(7, 75);
		ASSERT_EQUAL(string_combine(c),
		  U"{\n"
		  U"	75,\n"
		  U"	75,\n"
		  U"	75,\n"
		  U"	75,\n"
		  U"	75,\n"
		  U"	75,\n"
		  U"	75\n"
		  U"}"
		);
		// Assign larger array
		a = c;
		ASSERT_EQUAL(string_combine(a),
		  U"{\n"
		  U"	75,\n"
		  U"	75,\n"
		  U"	75,\n"
		  U"	75,\n"
		  U"	75,\n"
		  U"	75,\n"
		  U"	75\n"
		  U"}"
		);
		ASSERT_EQUAL(a, c);
		ASSERT_NOT_EQUAL(a, b);
		// Assign smaller array
		c = b;
		ASSERT_EQUAL(string_combine(c),
		  U"{\n"
		  U"	200,\n"
		  U"	85,\n"
		  U"	100000,\n"
		  U"	-100\n"
		  U"}"
		);
		ASSERT_EQUAL(c, b);
		ASSERT_NOT_EQUAL(a, c);
	}
END_TEST
