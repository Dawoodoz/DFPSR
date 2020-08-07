
#include "../testTools.h"

// TODO: Cannot use casting to parent type at the same time as automatic conversion from const char*
//       Cover everything using a single dsr::String type?
//       Use "" operand as only constructor?
void fooInPlace(dsr::String& target, const dsr::ReadableString& a, const dsr::ReadableString& b) {
	target.clear();
	target.append(U"Foo(");
	target.append(a);
	target.appendChar(U',');
	target.append(b);
	target.appendChar(U')');
}

dsr::String foo(const dsr::ReadableString& a, const dsr::ReadableString& b) {
	dsr::String result;
	result.reserve(string_length(a) + string_length(b) + 6);
	fooInPlace(result, a, b);
	return result;
}

START_TEST(String)
	{ // Length
		ASSERT_EQUAL(string_length(dsr::String()), 0);
		ASSERT_EQUAL(string_length(U""), 0);
		ASSERT_EQUAL(string_length(U"a"), 1);
		ASSERT_EQUAL(string_length(U"ab"), 2);
		ASSERT_EQUAL(string_length(U"abc"), 3);
		ASSERT_EQUAL(string_length(U"0123456789"), 10);
	}
	{ // Comparison
		dsr::ReadableString litA = U"Testing \u0444";
		dsr::ReadableString litB = U"Testing ф";
		ASSERT(string_match(litA, litB));
		ASSERT(!string_match(litA, U"wrong"));
		ASSERT(!string_match(U"wrong", litB));
		ASSERT(dsr::string_caseInsensitiveMatch(U"abc 123!", U"ABC 123!"));
		ASSERT(!dsr::string_caseInsensitiveMatch(U"abc 123!", U"ABD 123!"));
		ASSERT(dsr::string_match(U"aBc 123!", U"aBc 123!"));
		ASSERT(!dsr::string_match(U"abc 123!", U"ABC 123!"));
	}
	{ // Concatenation
		dsr::String ab = dsr::string_combine(U"a", U"b");
		ASSERT_MATCH(ab, U"ab");
		dsr::String cd = dsr::string_combine(U"c", U"d");
		ASSERT_MATCH(cd, U"cd");
		cd = dsr::string_combine(U"c", U"d");
		ASSERT_MATCH(cd, U"cd");
		auto abcd = ab + cd;
		ASSERT_MATCH(abcd, U"abcd");
		ASSERT_MATCH(dsr::string_combine(U"a", U"b", U"c", "d"), U"abcd");
	}
	{ // Sub-strings
		dsr::ReadableString abcd = U"abcd";
		dsr::String efgh = U"efgh";
		ASSERT_MATCH(dsr::string_inclusiveRange(abcd, 0, 3), U"abcd");
		ASSERT_MATCH(dsr::string_exclusiveRange(abcd, 1, 2), U"b");
		ASSERT_MATCH(dsr::string_inclusiveRange(efgh, 2, 3), U"gh");
		ASSERT_MATCH(dsr::string_exclusiveRange(efgh, 3, 4), U"h");
		ASSERT_MATCH(dsr::string_combine(string_from(abcd, 2), string_before(efgh, 2)), U"cdef");
		ASSERT_MATCH(dsr::string_exclusiveRange(abcd, 0, 0), U""); // No size returns nothing
		ASSERT_MATCH(dsr::string_exclusiveRange(abcd, -1, -2), U""); // A negative size doesn't have to be inside
		ASSERT_CRASH(dsr::string_inclusiveRange(abcd, -1, -1)); // Index below bound expected
		ASSERT_CRASH(dsr::string_inclusiveRange(abcd, 4, 4)); // Index above bound expected
	}
	{ // Processing
		dsr::String buffer = U"Garbage";
		ASSERT_MATCH(buffer, U"Garbage");
		buffer = foo(U"Ball", U"åäöÅÄÖ"); // Crash!
		ASSERT_MATCH(buffer, U"Foo(Ball,åäöÅÄÖ)"); // Failed
		fooInPlace(buffer, U"Å", U"ф");
		ASSERT_MATCH(buffer, U"Foo(Å,ф)");
	}
	{ // Numbers
		uint32_t x = 0;
		int32_t y = -123456;
		int64_t z = 100200300400500600ULL;
		dsr::String values = dsr::string_combine(U"x = ", x, U", y = ", y, U", z = ", z);
		ASSERT_MATCH(values, U"x = 0, y = -123456, z = 100200300400500600");
	}
	// Upper case
	ASSERT_MATCH(dsr::string_upperCase(U"a"), U"A");
	ASSERT_MATCH(dsr::string_upperCase(U"aB"), U"AB");
	ASSERT_MATCH(dsr::string_upperCase(U"abc"), U"ABC");
	ASSERT_MATCH(dsr::string_upperCase(U"abc1"), U"ABC1");
	ASSERT_MATCH(dsr::string_upperCase(U"Abc12"), U"ABC12");
	ASSERT_MATCH(dsr::string_upperCase(U"ABC123"), U"ABC123");
	// Lower case
	ASSERT_MATCH(dsr::string_lowerCase(U"a"), U"a");
	ASSERT_MATCH(dsr::string_lowerCase(U"aB"), U"ab");
	ASSERT_MATCH(dsr::string_lowerCase(U"abc"), U"abc");
	ASSERT_MATCH(dsr::string_lowerCase(U"abc1"), U"abc1");
	ASSERT_MATCH(dsr::string_lowerCase(U"Abc12"), U"abc12");
	ASSERT_MATCH(dsr::string_lowerCase(U"ABC123"), U"abc123");
	// Complete white space removal
	ASSERT_MATCH(dsr::string_removeAllWhiteSpace(U" "), U"");
	ASSERT_MATCH(dsr::string_removeAllWhiteSpace(U" abc\n	"), U"abc");
	ASSERT_MATCH(dsr::string_removeAllWhiteSpace(U" a \f sentence \r surrounded	\n by space	\a"), U"asentencesurroundedbyspace");
	// White space removal by pointing to a section of the original input
	ASSERT_MATCH(dsr::string_removeOuterWhiteSpace(U" "), U"");
	ASSERT_MATCH(dsr::string_removeOuterWhiteSpace(U"  abc  "), U"abc");
	ASSERT_MATCH(dsr::string_removeOuterWhiteSpace(U"  two words  "), U"two words");
	ASSERT_MATCH(dsr::string_removeOuterWhiteSpace(U"  \" something quoted \"  "), U"\" something quoted \"");
	// Quote mangling
	ASSERT_MATCH(dsr::string_mangleQuote(U""), U"\"\"");
	ASSERT_MATCH(dsr::string_mangleQuote(U"1"), U"\"1\"");
	ASSERT_MATCH(dsr::string_mangleQuote(U"12"), U"\"12\"");
	ASSERT_MATCH(dsr::string_mangleQuote(U"123"), U"\"123\"");
	ASSERT_MATCH(dsr::string_mangleQuote(U"abc"), U"\"abc\"");
	// Not enough quote signs
	ASSERT_CRASH(dsr::string_unmangleQuote(U""));
	ASSERT_CRASH(dsr::string_unmangleQuote(U" "));
	ASSERT_CRASH(dsr::string_unmangleQuote(U"ab\"cd"));
	// Too many quote signs
	ASSERT_CRASH(dsr::string_unmangleQuote(U"ab\"cd\"ef\"gh"));
	// Basic quote
	ASSERT_MATCH(dsr::string_unmangleQuote(U"\"ab\""), U"ab");
	// Surrounded quote
	ASSERT_MATCH(dsr::string_unmangleQuote(U"\"ab\"cd"), U"ab");
	ASSERT_MATCH(dsr::string_unmangleQuote(U"ab\"cd\""), U"cd");
	ASSERT_MATCH(dsr::string_unmangleQuote(U"ab\"cd\"ef"), U"cd");
	// Mangled quote inside of quote
	ASSERT_MATCH(dsr::string_unmangleQuote(U"ab\"c\\\"d\"ef"), U"c\"d");
	ASSERT_MATCH(dsr::string_unmangleQuote(dsr::string_mangleQuote(U"c\"d")), U"c\"d");
	// Mangle things
	dsr::String randomText;
	for (int i = 1; i < 100; i++) {
		// Randomize previous characters
		for (int j = 1; j < i - 1; j++) {
			randomText.write(j, (DsrChar)((i * 21 + j * 49 + 136) % 1024));
		}
		// Add a new random character
		randomText.appendChar((i * 21 + 136) % 256);
		ASSERT_MATCH(dsr::string_unmangleQuote(dsr::string_mangleQuote(randomText)), randomText);
	}
	// Number serialization
	ASSERT_MATCH(dsr::string_combine(0, U" ", 1), U"0 1");
	ASSERT_MATCH(dsr::string_combine(14, U"x", 135), U"14x135");
	ASSERT_MATCH(dsr::string_combine(-135), U"-135");
	ASSERT_MATCH(dsr::string_combine(-14), U"-14");
	ASSERT_MATCH(dsr::string_combine(-1), U"-1");
	ASSERT_MATCH(dsr::string_combine(0u), U"0");
	ASSERT_MATCH(dsr::string_combine(1u), U"1");
	ASSERT_MATCH(dsr::string_combine(14u), U"14");
	ASSERT_MATCH(dsr::string_combine(135u), U"135");
	// Number parsing
	ASSERT_EQUAL(string_toInteger(U"0"), 0);
	ASSERT_EQUAL(string_toInteger(U"-0"), 0);
	ASSERT_EQUAL(string_toInteger(U"No digits here."), 0);
	ASSERT_EQUAL(string_toInteger(U" (12 garbage 34) "), 1234); // You are supposed to catch these errors before converting to an integer
	ASSERT_EQUAL(string_toInteger(U""), 0);
	ASSERT_EQUAL(string_toInteger(U"1"), 1);
	ASSERT_EQUAL(string_toInteger(U"-1"), -1);
	ASSERT_EQUAL(string_toInteger(U"1024"), 1024);
	ASSERT_EQUAL(string_toInteger(U"-1024"), -1024);
	ASSERT_EQUAL(string_toInteger(U"1000000"), 1000000);
	ASSERT_EQUAL(string_toInteger(U"-1000000"), -1000000);
	ASSERT_EQUAL(string_toInteger(U"123"), 123);
	ASSERT_EQUAL(string_toDouble(U"123"), 123.0);
	ASSERT_EQUAL(string_toDouble(U"123.456"), 123.456);
	{ // Splitting
		List<ReadableString> result;
		string_split_inPlace(result, U"a.b.c.d", U'.');
		ASSERT_EQUAL(result.length(), 4);
		ASSERT_MATCH(result[0], U"a");
		ASSERT_MATCH(result[1], U"b");
		ASSERT_MATCH(result[2], U"c");
		ASSERT_MATCH(result[3], U"d");
		String content = U"One Two Three";
		result = string_split(content, U' ');
		ASSERT_EQUAL(result.length(), 3);
		ASSERT_MATCH(result[0], U"One");
		ASSERT_MATCH(result[1], U"Two");
		ASSERT_MATCH(result[2], U"Three");
		string_split_inPlace(result, U"Four.Five", U'.', true);
		ASSERT_EQUAL(result.length(), 5);
		ASSERT_MATCH(result[0], U"One");
		ASSERT_MATCH(result[1], U"Two");
		ASSERT_MATCH(result[2], U"Three");
		ASSERT_MATCH(result[3], U"Four");
		ASSERT_MATCH(result[4], U"Five");
		string_split_inPlace(result, U" 1 | 2 ", U'|');
		ASSERT_EQUAL(result.length(), 2);
		ASSERT_MATCH(result[0], U" 1 ");
		ASSERT_MATCH(result[1], U" 2 ");
	}
	// TODO: Test taking a part of a parent string with a start offset, leaving the parent scope,
	//       and expanding with append while the buffer isn't shared but has an offset from buffer start.
	// TODO: Assert that buffers are shared when they should, but prevents side-effects when one is being written to.
END_TEST
