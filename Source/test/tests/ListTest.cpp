
#include "../testTools.h"

START_TEST(List)
	{ // Fixed size elements
		List<int> myIntegers;
		ASSERT_EQUAL(myIntegers.length(), 0);
		for (int i = 0; i < 1000; i++) {
			myIntegers.push(i * 2 + 1); // 1, 3, 5, 7, 9, 11, 13...
		}
		ASSERT_EQUAL(myIntegers.length(), 1000);
		int integerErrorCount = 0;
		for (int i = 0; i < 1000; i++) {
			if (myIntegers[i] != i * 2 + 1) { // 1, 3, 5, 7, 9, 11, 13...
				integerErrorCount++;
			}
		}
		ASSERT_EQUAL(integerErrorCount, 0);
	}
	{ // Complex elements
		List<String> myStrings;
		ASSERT_EQUAL(myStrings.length(), 0);
		myStrings.pushConstruct(U"is");
		myStrings.push(U"this");
		myStrings.push(U"a");
		myStrings.push(U"list");
		ASSERT_EQUAL(myStrings.length(), 4);
		ASSERT_MATCH(myStrings[0], U"is");
		ASSERT_MATCH(myStrings[1], U"this");
		ASSERT_MATCH(myStrings[2], U"a");
		ASSERT_MATCH(myStrings[3], U"list");
		ASSERT_MATCH(myStrings.first(), U"is");
		ASSERT_MATCH(myStrings.last(), U"list");
		myStrings.swap(0, 1);
		ASSERT_EQUAL(myStrings.length(), 4);
		ASSERT_MATCH(myStrings[0], U"this");
		ASSERT_MATCH(myStrings[1], U"is");
		ASSERT_MATCH(myStrings[2], U"a");
		ASSERT_MATCH(myStrings[3], U"list");
		List<String> myOtherStrings = myStrings;
		myStrings.remove(1);
		ASSERT_EQUAL(myStrings.length(), 3);
		ASSERT_MATCH(myStrings[0], U"this");
		ASSERT_MATCH(myStrings[1], U"a");
		ASSERT_MATCH(myStrings[2], U"list");
		myStrings.remove(0);
		ASSERT_EQUAL(myStrings.length(), 2);
		ASSERT_MATCH(myStrings[0], U"a");
		ASSERT_MATCH(myStrings[1], U"list");
		myStrings.pop();
		ASSERT_EQUAL(myStrings.length(), 1);
		ASSERT_MATCH(myStrings[0], U"a");
		myStrings.clear();
		ASSERT_EQUAL(myStrings.length(), 0);
		ASSERT_EQUAL(myOtherStrings.length(), 4);
		ASSERT_MATCH(myOtherStrings[0], U"this");
		ASSERT_MATCH(myOtherStrings[1], U"is");
		ASSERT_MATCH(myOtherStrings[2], U"a");
		ASSERT_MATCH(myOtherStrings[3], U"list");
		myOtherStrings.clear();
		ASSERT_EQUAL(myOtherStrings.length(), 0);
	}
END_TEST
