
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
		ASSERT_EQUAL(myStrings[0], U"is");
		ASSERT_EQUAL(myStrings[1], U"this");
		ASSERT_EQUAL(myStrings[2], U"a");
		ASSERT_EQUAL(myStrings[3], U"list");
		ASSERT_EQUAL(myStrings.first(), U"is");
		ASSERT_EQUAL(myStrings.last(), U"list");
		ASSERT_EQUAL(myStrings, List<String>(U"is", U"this", U"a", U"list"));
		myStrings.swap(0, 1);
		ASSERT_EQUAL(myStrings.length(), 4);
		ASSERT_EQUAL(myStrings[0], U"this");
		ASSERT_EQUAL(myStrings[1], U"is");
		ASSERT_EQUAL(myStrings[2], U"a");
		ASSERT_EQUAL(myStrings[3], U"list");
		ASSERT_EQUAL(myStrings, List<String>(U"this", U"is", U"a", U"list"));
		List<String> myOtherStrings = myStrings;
		myStrings.remove(1);
		ASSERT_EQUAL(myStrings.length(), 3);
		ASSERT_EQUAL(myStrings[0], U"this");
		ASSERT_EQUAL(myStrings[1], U"a");
		ASSERT_EQUAL(myStrings[2], U"list");
		ASSERT_EQUAL(myStrings, List<String>(U"this", U"a", U"list"));
		myStrings.remove(0);
		ASSERT_EQUAL(myStrings.length(), 2);
		ASSERT_EQUAL(myStrings[0], U"a");
		ASSERT_EQUAL(myStrings[1], U"list");
		ASSERT_EQUAL(myStrings, List<String>(U"a", U"list"));
		myStrings.pop();
		ASSERT_EQUAL(myStrings.length(), 1);
		ASSERT_EQUAL(myStrings[0], U"a");
		ASSERT_EQUAL(myStrings, List<String>(U"a"));
		myStrings.clear();
		ASSERT_EQUAL(myStrings.length(), 0);
		ASSERT_EQUAL(myOtherStrings.length(), 4);
		ASSERT_EQUAL(myOtherStrings[0], U"this");
		ASSERT_EQUAL(myOtherStrings[1], U"is");
		ASSERT_EQUAL(myOtherStrings[2], U"a");
		ASSERT_EQUAL(myOtherStrings[3], U"list");
		ASSERT_EQUAL(myOtherStrings, List<String>(U"this", U"is", U"a", U"list"));
		myOtherStrings.clear();
		ASSERT_EQUAL(myOtherStrings.length(), 0);
	}
END_TEST
