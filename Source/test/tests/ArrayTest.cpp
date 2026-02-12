
#include "../testTools.h"
#include "../../DFPSR/collection/Array.h"

START_TEST(Array)
	{ // Arrays of integers.
		ASSERT_HEAP_DEPTH(0);
		Array<int> a = Array<int>(4, 123);
		ASSERT_HEAP_DEPTH(1);
		a[1] = 85;
		a[3] = -100;
		ASSERT_EQUAL(a.length(), 4);
		ASSERT_EQUAL(a[0], 123);
		ASSERT_EQUAL(a[1], 85);
		ASSERT_EQUAL(a[2], 123);
		ASSERT_EQUAL(a[3], -100);
		ASSERT_HEAP_DEPTH(1);
		ASSERT_EQUAL(string_combine(a),
		  U"{\n"
		  U"	123,\n"
		  U"	85,\n"
		  U"	123,\n"
		  U"	-100\n"
		  U"}"
		);
		ASSERT_HEAP_DEPTH(1);
		// An initial assignment uses the copy constructor, because there is no pre-existing data in b.
		Array<int> b = a;
		ASSERT_HEAP_DEPTH(2);
		b[0] = 200;
		b[2] = 100000;
		ASSERT_HEAP_DEPTH(2);
		// The b array has changed...
		ASSERT_EQUAL(string_combine(b),
		  U"{\n"
		  U"	200,\n"
		  U"	85,\n"
		  U"	100000,\n"
		  U"	-100\n"
		  U"}"
		);
		ASSERT_HEAP_DEPTH(2);
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
		ASSERT_HEAP_DEPTH(2);
		// Assigning from copy construction.
		a = Array<int>(b);
		ASSERT_HEAP_DEPTH(2);
		// Now they are equal
		ASSERT_EQUAL(a, b);
		// Create another length
		Array<int> c = Array<int>(7, 75);
		ASSERT_HEAP_DEPTH(3);
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
		ASSERT_HEAP_DEPTH(3);
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
		ASSERT_HEAP_DEPTH(3);
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
	{ // Arrays of non-trivial types.
		Array<Array<String>> a = Array<Array<String>>(2, Array<String>(3, U"?"));
		ASSERT_EQUAL(a.length(), 2);
		ASSERT_CRASH(a[-1], U"Array index -1 is out of bound 0..1!");
		ASSERT_EQUAL(a[0].length(), 3);
		ASSERT_EQUAL(a[1].length(), 3);		
		ASSERT_CRASH(a[2], U"Array index 2 is out of bound 0..1!");
		ASSERT_EQUAL(a[0][0], U"?");
		ASSERT_EQUAL(a[0][1], U"?");
		ASSERT_EQUAL(a[0][2], U"?");		
		ASSERT_EQUAL(a[1][0], U"?");
		ASSERT_EQUAL(a[1][1], U"?");
		ASSERT_EQUAL(a[1][2], U"?");
		a[0][0] = U"Testing";
		a[0][1] = U"an";
		a[0][2] = U"array";
		a[1][0] = U"of";
		a[1][1] = U"string";
		a[1][2] = U"arrays";
		ASSERT_EQUAL(a[0][0], U"Testing");
		ASSERT_EQUAL(a[0][1], U"an");
		ASSERT_EQUAL(a[0][2], U"array");		
		ASSERT_EQUAL(a[1][0], U"of");
		ASSERT_EQUAL(a[1][1], U"string");
		ASSERT_EQUAL(a[1][2], U"arrays");
		Array<String> b = dsr::move(a[0]);
		ASSERT_EQUAL(a[0].length(), 0);
		ASSERT_EQUAL(a[1].length(), 3);
		ASSERT_EQUAL(b.length(), 3);
		Array<String> c(dsr::move(a[1]));
		ASSERT_EQUAL(a[0].length(), 0);
		ASSERT_EQUAL(a[1].length(), 0);
		ASSERT_EQUAL(b.length(), 3);
		ASSERT_EQUAL(c.length(), 3);
	}
END_TEST
