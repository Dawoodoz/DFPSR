
#include "../testTools.h"
#include "../../DFPSR/collection/FixedArray.h"

START_TEST(FixedArray)
	{ // Fixed arrays of integers.
		ASSERT_HEAP_DEPTH(0);
		FixedArray<int, 4> a = FixedArray<int, 4>(123);
		ASSERT_HEAP_DEPTH(0);
		a[1] = 85;
		a[3] = -100;
		ASSERT_EQUAL(a.length(), 4);
		ASSERT_EQUAL(a[0], 123);
		ASSERT_EQUAL(a[1], 85);
		ASSERT_EQUAL(a[2], 123);
		ASSERT_EQUAL(a[3], -100);
		ASSERT_HEAP_DEPTH(0);
		ASSERT_EQUAL(string_combine(a),
		  U"{\n"
		  U"	123,\n"
		  U"	85,\n"
		  U"	123,\n"
		  U"	-100\n"
		  U"}"
		);
		ASSERT_HEAP_DEPTH(0);
		// Copy from one fixed size array to another of the same size.
		FixedArray<int, a.length()> b = a;
		ASSERT_EQUAL(b.length(), 4);
		ASSERT_HEAP_DEPTH(0);
		b[0] = 200;
		b[2] = 100000;
		ASSERT_HEAP_DEPTH(0);
		// The b array has changed...
		ASSERT_EQUAL(string_combine(b),
		  U"{\n"
		  U"	200,\n"
		  U"	85,\n"
		  U"	100000,\n"
		  U"	-100\n"
		  U"}"
		);
		ASSERT_HEAP_DEPTH(0);
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
		ASSERT_HEAP_DEPTH(0);
		// Assigning from copy construction.
		a = FixedArray<int, 4>(b);
		ASSERT_HEAP_DEPTH(0);
		// Now they are equal
		ASSERT_EQUAL(a, b);
	}
	{ // Fixed arrays of non-trivial types.
		FixedArray<FixedArray<String, 3>, 2> a = FixedArray<FixedArray<String, 3>, 2>(FixedArray<String, 3>(U"?"));
		ASSERT_EQUAL(a.length(), 2);
		ASSERT_CRASH(a[-1], U"FixedArray index -1 is out of bound 0..1!");
		ASSERT_EQUAL(a[0].length(), 3);
		ASSERT_EQUAL(a[1].length(), 3);		
		ASSERT_CRASH(a[2], U"FixedArray index 2 is out of bound 0..1!");
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
	}
END_TEST
