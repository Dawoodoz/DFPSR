
#include "../testTools.h"
#include "../../DFPSR/api/stringAPI.h"

void fooInPlace(dsr::String& target, const dsr::ReadableString& a, const dsr::ReadableString& b) {
	string_clear(target);
	string_append(target, U"Foo(");
	string_append(target, a);
	string_appendChar(target, U',');
	string_append(target, b);
	string_appendChar(target, U')');
}

dsr::String foo(const dsr::ReadableString& a, const dsr::ReadableString& b) {
	dsr::String result;
	string_reserve(result, string_length(a) + string_length(b) + 6);
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
	{ // Reading characters
		ASSERT_EQUAL(dsr::ReadableString(U"ABC")[0], U'A');
		ASSERT_EQUAL(dsr::ReadableString(U"ABC")[1], U'B');
		ASSERT_EQUAL(dsr::ReadableString(U"ABC")[2], U'C');
		ASSERT_EQUAL(dsr::ReadableString(U"ABC")[3], U'\0');
		ASSERT_EQUAL(dsr::ReadableString(U"ABC")[10], U'\0');
		ASSERT_EQUAL(dsr::ReadableString(U"ABC")[1000000], U'\0');
		ASSERT_EQUAL(dsr::ReadableString(U"ABC")[-1], U'\0');
		ASSERT_EQUAL(dsr::ReadableString(U"ABC")[-1000000], U'\0');
		ASSERT_EQUAL(dsr::ReadableString(U"")[-1], U'\0');
		ASSERT_EQUAL(dsr::ReadableString(U"")[0], U'\0');
		ASSERT_EQUAL(dsr::ReadableString(U"")[1], U'\0');
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
		ASSERT_EQUAL(ab, U"ab");
		dsr::String cd = dsr::string_combine(U"c", U"d");
		ASSERT_EQUAL(cd, U"cd");
		cd = dsr::string_combine(U"c", U"d");
		ASSERT_EQUAL(cd, U"cd");
		auto abcd = ab + cd;
		ASSERT_EQUAL(abcd, U"abcd");
		ASSERT_EQUAL(dsr::string_combine(U"a", U"b", U"c", "d"), U"abcd");
	}
	{ // Sub-strings
		dsr::ReadableString abcd = U"abcd";
		dsr::String efgh = U"efgh";
		ASSERT_EQUAL(dsr::string_inclusiveRange(abcd, 0, 3), U"abcd");
		ASSERT_EQUAL(dsr::string_exclusiveRange(abcd, 1, 2), U"b");
		ASSERT_EQUAL(dsr::string_inclusiveRange(efgh, 2, 3), U"gh");
		ASSERT_EQUAL(dsr::string_exclusiveRange(efgh, 3, 4), U"h");
		ASSERT_EQUAL(dsr::string_combine(string_from(abcd, 2), string_before(efgh, 2)), U"cdef");
		ASSERT_EQUAL(dsr::string_exclusiveRange(abcd, 0, 0), U""); // No size returns nothing
		ASSERT_EQUAL(dsr::string_exclusiveRange(abcd, -670214452, 2), U"ab"); // Reading out of bound is clamped
		ASSERT_EQUAL(dsr::string_exclusiveRange(abcd, 2, 985034841), U"cd"); // Reading out of bound is clamped
		ASSERT_EQUAL(dsr::string_exclusiveRange(abcd, 4, 764), U""); // Completely ous of bound returns nothing
		ASSERT_EQUAL(dsr::string_exclusiveRange(abcd, -631, 0), U""); // Completely ous of bound returns nothing
	}
	{ // Processing
		dsr::String buffer = U"Garbage";
		ASSERT_EQUAL(buffer, U"Garbage");
		buffer = foo(U"Ball", U"åäöÅÄÖ");
		ASSERT_EQUAL(buffer, U"Foo(Ball,åäöÅÄÖ)");
		fooInPlace(buffer, U"Å", U"ф");
		ASSERT_EQUAL(buffer, U"Foo(Å,ф)");
	}
	{ // Numbers
		uint32_t x = 0;
		int32_t y = -123456;
		int64_t z = 100200300400500600ULL;
		dsr::String values = dsr::string_combine(U"x = ", x, U", y = ", y, U", z = ", z);
		ASSERT_EQUAL(values, U"x = 0, y = -123456, z = 100200300400500600");
	}
	{ // Identifying numbers
		ASSERT_EQUAL(dsr::character_isDigit(U'0' - 1), false);
		ASSERT_EQUAL(dsr::character_isDigit(U'0'), true);
		ASSERT_EQUAL(dsr::character_isDigit(U'1'), true);
		ASSERT_EQUAL(dsr::character_isDigit(U'2'), true);
		ASSERT_EQUAL(dsr::character_isDigit(U'3'), true);
		ASSERT_EQUAL(dsr::character_isDigit(U'4'), true);
		ASSERT_EQUAL(dsr::character_isDigit(U'5'), true);
		ASSERT_EQUAL(dsr::character_isDigit(U'6'), true);
		ASSERT_EQUAL(dsr::character_isDigit(U'7'), true);
		ASSERT_EQUAL(dsr::character_isDigit(U'8'), true);
		ASSERT_EQUAL(dsr::character_isDigit(U'9'), true);
		ASSERT_EQUAL(dsr::character_isDigit(U'9' + 1), false);
		ASSERT_EQUAL(dsr::character_isDigit(U'a'), false);
		ASSERT_EQUAL(dsr::character_isDigit(U' '), false);
		ASSERT_EQUAL(dsr::character_isIntegerCharacter(U'-'), true);
		ASSERT_EQUAL(dsr::character_isIntegerCharacter(U'0' - 1), false);
		ASSERT_EQUAL(dsr::character_isIntegerCharacter(U'0'), true);
		ASSERT_EQUAL(dsr::character_isIntegerCharacter(U'9'), true);
		ASSERT_EQUAL(dsr::character_isIntegerCharacter(U'9' + 1), false);
		ASSERT_EQUAL(dsr::character_isIntegerCharacter(U'a'), false);
		ASSERT_EQUAL(dsr::character_isIntegerCharacter(U' '), false);
		ASSERT_EQUAL(dsr::character_isValueCharacter(U'-'), true);
		ASSERT_EQUAL(dsr::character_isValueCharacter(U'.'), true);
		ASSERT_EQUAL(dsr::character_isValueCharacter(U'0' - 1), false);
		ASSERT_EQUAL(dsr::character_isValueCharacter(U'0'), true);
		ASSERT_EQUAL(dsr::character_isValueCharacter(U'9'), true);
		ASSERT_EQUAL(dsr::character_isValueCharacter(U'9' + 1), false);
		ASSERT_EQUAL(dsr::character_isValueCharacter(U'a'), false);
		ASSERT_EQUAL(dsr::character_isValueCharacter(U' '), false);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U' '), true);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U'\t'), true);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U'\r'), true);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U'\0'), false);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U'a'), false);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U'1'), false);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U'('), false);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U')'), false);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U'.'), false);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U','), false);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U'-'), false);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U'_'), false);
		ASSERT_EQUAL(dsr::character_isWhiteSpace(U'|'), false);
		ASSERT_EQUAL(dsr::string_isInteger(U"0"), true);
		ASSERT_EQUAL(dsr::string_isInteger(U"1"), true);
		ASSERT_EQUAL(dsr::string_isInteger(U"-0"), true);
		ASSERT_EQUAL(dsr::string_isInteger(U"-1"), true);
		ASSERT_EQUAL(dsr::string_isInteger(U"0", false), true);
		ASSERT_EQUAL(dsr::string_isInteger(U" 0 "), true);
		ASSERT_EQUAL(dsr::string_isInteger(U" 0 ", false), false);
		ASSERT_EQUAL(dsr::string_isInteger(U" 123"), true);
		ASSERT_EQUAL(dsr::string_isInteger(U"-123"), true);
		ASSERT_EQUAL(dsr::string_isInteger(U""), false);
		ASSERT_EQUAL(dsr::string_isInteger(U"85x"), false);
		ASSERT_EQUAL(dsr::string_isInteger(U"F15"), false);
		ASSERT_EQUAL(dsr::string_isInteger(U" 14"), true);
		ASSERT_EQUAL(dsr::string_isInteger(U"8 "), true);
		ASSERT_EQUAL(dsr::string_isInteger(U"		100"), true);
		ASSERT_EQUAL(dsr::string_isInteger(U"100		"), true);
		ASSERT_EQUAL(dsr::string_isInteger(U"10 10"), false);
		ASSERT_EQUAL(dsr::string_isInteger(U"10		10"), false);
		ASSERT_EQUAL(dsr::string_isInteger(U" 10  10 "), false);
		ASSERT_EQUAL(dsr::string_isDouble(U"0"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"-0"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"1"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"-1"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"1.1"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"-1.1"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U".1"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"-.1"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"0", false), true);
		ASSERT_EQUAL(dsr::string_isDouble(U" 0 "), true);
		ASSERT_EQUAL(dsr::string_isDouble(U" 0 ", false), false);
		ASSERT_EQUAL(dsr::string_isDouble(U" 123"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"-123"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"0.5"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"-0.5"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U".5"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"-.5"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"0.54321"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U"-0.54321"), true);
		ASSERT_EQUAL(dsr::string_isDouble(U""), false);
		ASSERT_EQUAL(dsr::string_isDouble(U"0..0"), false);
		ASSERT_EQUAL(dsr::string_isDouble(U"M0.0"), false);
		ASSERT_EQUAL(dsr::string_isDouble(U"0.0x"), false);
		ASSERT_EQUAL(dsr::string_isDouble(U"T0.0q"), false);
	}
	// Upper case
	ASSERT_EQUAL(dsr::string_upperCase(U"a"), U"A");
	ASSERT_EQUAL(dsr::string_upperCase(U"aB"), U"AB");
	ASSERT_EQUAL(dsr::string_upperCase(U"abc"), U"ABC");
	ASSERT_EQUAL(dsr::string_upperCase(U"abc1"), U"ABC1");
	ASSERT_EQUAL(dsr::string_upperCase(U"Abc12"), U"ABC12");
	ASSERT_EQUAL(dsr::string_upperCase(U"ABC123"), U"ABC123");
	// Lower case
	ASSERT_EQUAL(dsr::string_lowerCase(U"a"), U"a");
	ASSERT_EQUAL(dsr::string_lowerCase(U"aB"), U"ab");
	ASSERT_EQUAL(dsr::string_lowerCase(U"abc"), U"abc");
	ASSERT_EQUAL(dsr::string_lowerCase(U"abc1"), U"abc1");
	ASSERT_EQUAL(dsr::string_lowerCase(U"Abc12"), U"abc12");
	ASSERT_EQUAL(dsr::string_lowerCase(U"ABC123"), U"abc123");
	// White space removal by pointing to a section of the original input
	ASSERT_EQUAL(dsr::string_removeOuterWhiteSpace(U" "), U"");
	ASSERT_EQUAL(dsr::string_removeOuterWhiteSpace(U"  abc  "), U"abc");
	ASSERT_EQUAL(dsr::string_removeOuterWhiteSpace(U"  two words  "), U"two words");
	ASSERT_EQUAL(dsr::string_removeOuterWhiteSpace(U"  \" something quoted \"  "), U"\" something quoted \"");
	// Quote mangling
	ASSERT_EQUAL(dsr::string_mangleQuote(U""), U"\"\"");
	ASSERT_EQUAL(dsr::string_mangleQuote(U"1"), U"\"1\"");
	ASSERT_EQUAL(dsr::string_mangleQuote(U"12"), U"\"12\"");
	ASSERT_EQUAL(dsr::string_mangleQuote(U"123"), U"\"123\"");
	ASSERT_EQUAL(dsr::string_mangleQuote(U"abc"), U"\"abc\"");
	// Not enough quote signs
	ASSERT_CRASH(dsr::string_unmangleQuote(U""), U"Cannot unmangle using string_unmangleQuote without beginning and ending with quote signs!");
	ASSERT_CRASH(dsr::string_unmangleQuote(U" "), U"Cannot unmangle using string_unmangleQuote without beginning and ending with quote signs!");
	ASSERT_CRASH(dsr::string_unmangleQuote(U"ab\"cd"), U"Cannot unmangle using string_unmangleQuote without beginning and ending with quote signs!");
	// Too many quote signs
	ASSERT_CRASH(dsr::string_unmangleQuote(U"ab\"cd\"ef\"gh"), U"Unmangled double quote sign detected in string_unmangleQuote!");
	// Basic quote
	ASSERT_EQUAL(dsr::string_unmangleQuote(U"\"ab\""), U"ab");
	// Surrounded quote
	ASSERT_EQUAL(dsr::string_unmangleQuote(U"\"ab\"cd"), U"ab");
	ASSERT_EQUAL(dsr::string_unmangleQuote(U"ab\"cd\""), U"cd");
	ASSERT_EQUAL(dsr::string_unmangleQuote(U"ab\"cd\"ef"), U"cd");
	// Mangled quote inside of quote
	ASSERT_EQUAL(dsr::string_unmangleQuote(U"ab\"c\\\"d\"ef"), U"c\"d");
	ASSERT_EQUAL(dsr::string_unmangleQuote(dsr::string_mangleQuote(U"c\"d")), U"c\"d");
	// Mangle things
	dsr::String randomText;
	string_reserve(randomText, 100);
	for (int i = 1; i < 100; i++) {
		// Randomize previous characters
		for (int j = 1; j < i - 1; j++) {
			string_appendChar(randomText, (DsrChar)((i * 21 + j * 49 + 136) % 1024));
		}
		// Add a new random character
		string_appendChar(randomText, (i * 21 + 136) % 256);
		ASSERT_EQUAL(dsr::string_unmangleQuote(dsr::string_mangleQuote(randomText)), randomText);
	}
	// Number serialization
	ASSERT_EQUAL(dsr::string_combine(0, U" ", 1), U"0 1");
	ASSERT_EQUAL(dsr::string_combine(14, U"x", 135), U"14x135");
	ASSERT_EQUAL(dsr::string_combine(-135), U"-135");
	ASSERT_EQUAL(dsr::string_combine(-14), U"-14");
	ASSERT_EQUAL(dsr::string_combine(-1), U"-1");
	ASSERT_EQUAL(dsr::string_combine(0u), U"0");
	ASSERT_EQUAL(dsr::string_combine(1u), U"1");
	ASSERT_EQUAL(dsr::string_combine(14u), U"14");
	ASSERT_EQUAL(dsr::string_combine(135u), U"135");
	ASSERT_EQUAL(dsr::string_combine(0.0), U"0.0");
	ASSERT_EQUAL(dsr::string_combine(-0.0), U"0.0");
	ASSERT_EQUAL(dsr::string_combine(1.0), U"1.0");
	ASSERT_EQUAL(dsr::string_combine(10.0), U"10.0");
	ASSERT_EQUAL(dsr::string_combine(100.0), U"100.0");
	ASSERT_EQUAL(dsr::string_combine(1000.0), U"1000.0");
	ASSERT_EQUAL(dsr::string_combine(10000.0), U"10000.0");
	ASSERT_EQUAL(dsr::string_combine(100000.0), U"100000.0");
	ASSERT_EQUAL(dsr::string_combine(1000000.0), U"1000000.0");
	ASSERT_EQUAL(dsr::string_combine(-1.0), U"-1.0");
	ASSERT_EQUAL(dsr::string_combine(-10.0), U"-10.0");
	ASSERT_EQUAL(dsr::string_combine(-100.0), U"-100.0");
	ASSERT_EQUAL(dsr::string_combine(-1000.0), U"-1000.0");
	ASSERT_EQUAL(dsr::string_combine(-10000.0), U"-10000.0");
	ASSERT_EQUAL(dsr::string_combine(-100000.0), U"-100000.0");
	ASSERT_EQUAL(dsr::string_combine(-1000000.0), U"-1000000.0");
	ASSERT_EQUAL(dsr::string_combine(0.5), U"0.5");
	ASSERT_EQUAL(dsr::string_combine(-0.5), U"-0.5");
	ASSERT_EQUAL(dsr::string_combine(789.123456), U"789.123456");
	ASSERT_EQUAL(dsr::string_combine(-789.123456), U"-789.123456");
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
	{ // Assigning strings using reference counting
		String a = U"Some text";
		ASSERT_EQUAL(string_getBufferUseCount(a), 1);
		String b = a;
		ASSERT_EQUAL(string_getBufferUseCount(a), 2);
		ASSERT_EQUAL(string_getBufferUseCount(b), 2);
		String c = b;
		ASSERT_EQUAL(string_getBufferUseCount(a), 3);
		ASSERT_EQUAL(string_getBufferUseCount(b), 3);
		ASSERT_EQUAL(string_getBufferUseCount(c), 3);
	}
	{ // String splitting by shared reference counted buffer
		String source = U" a . b . c . d ";
		String source2 = U" a . b .\tc ";
		ASSERT_EQUAL(string_getBufferUseCount(source), 1);
		ASSERT_EQUAL(string_getBufferUseCount(source2), 1);
		List<String> result;
		result = string_split(source, U'.', false);
		ASSERT_EQUAL(result.length(), 4);
		ASSERT_EQUAL(result[0], U" a ");
		ASSERT_EQUAL(result[1], U" b ");
		ASSERT_EQUAL(result[2], U" c ");
		ASSERT_EQUAL(result[3], U" d ");
		ASSERT_EQUAL(string_getBufferUseCount(source), 5);
		ASSERT_EQUAL(string_getBufferUseCount(source2), 1);
		result = string_split(source2, U'.', true);
		ASSERT_EQUAL(result.length(), 3);
		ASSERT_EQUAL(result[0], U"a");
		ASSERT_EQUAL(result[1], U"b");
		ASSERT_EQUAL(result[2], U"c");
		ASSERT_EQUAL(string_getBufferUseCount(source), 1);
		ASSERT_EQUAL(string_getBufferUseCount(source2), 4);
	}
	{ // Automatically allocating a shared buffer for many elements
		// Splitting String shares memory.
		String original = U" a . b . c . d ";
		List<String> result = string_split(original, U'.', true);
		ASSERT_EQUAL(result[0], U"a");
		ASSERT_EQUAL(result[1], U"b");
		ASSERT_EQUAL(result[2], U"c");
		ASSERT_EQUAL(result[3], U"d");
		ASSERT_EQUAL(string_getBufferUseCount(original), 5);
		ASSERT_EQUAL(string_getBufferUseCount(result[0]), 5);
		ASSERT_EQUAL(string_getBufferUseCount(result[1]), 5);
		ASSERT_EQUAL(string_getBufferUseCount(result[2]), 5);
		ASSERT_EQUAL(string_getBufferUseCount(result[3]), 5);
		// Splitting a literal allocates no string buffers.
		result = string_split(U" a . b . c ", U'.', false);
		ASSERT_EQUAL(result[0], U" a ");
		ASSERT_EQUAL(result[1], U" b ");
		ASSERT_EQUAL(result[2], U" c ");
		ASSERT_EQUAL(string_getBufferUseCount(result[0]), 0);
		ASSERT_EQUAL(string_getBufferUseCount(result[1]), 0);
		ASSERT_EQUAL(string_getBufferUseCount(result[2]), 0);
	}
	{ // Callback splitting
		String numbers = U"1, 3, 5, 7, 9";
		List<int> result;
		string_split_callback([&numbers, &result](ReadableString section) {
			result.push(string_toInteger(section));
		}, numbers, U',');
		ASSERT_EQUAL(result.length(), 5);
		ASSERT_EQUAL(result[0], 1);
		ASSERT_EQUAL(result[1], 3);
		ASSERT_EQUAL(result[2], 5);
		ASSERT_EQUAL(result[3], 7);
		ASSERT_EQUAL(result[4], 9);
	}
	// TODO: Test taking a part of a parent string with a start offset, leaving the parent scope,
	//       and expanding with append while the buffer isn't shared but has an offset from buffer start.
	// TODO: Test sharing the same buffer between strings, then clear and append more text without overwriting other strings.
	// TODO: Assert that buffers are shared when they should, but prevents side-effects when one is being written to.
END_TEST
