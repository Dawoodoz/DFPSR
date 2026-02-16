
#include "../testTools.h"
#include "../../DFPSR/api/algorithmAPI_List.h"

START_TEST(ListAlgorithm)
	{ // List sorting with duplicate elements.
		List<int32_t> myList(5, 2, 18, 6, -1, 4, 6, -64, 2, 45);
		list_heapSort_ascending(myList);
		ASSERT_EQUAL(myList, List<int32_t>(-64, -1, 2, 2, 4, 5, 6, 6, 18, 45));
		list_heapSort_descending(myList);
		ASSERT_EQUAL(myList, List<int32_t>(45, 18, 6, 6, 5, 4, 2, 2, -1, -64));
	}
	{
		List<int32_t> unsortedSet(7, 5, 2, 4);
		ASSERT_EQUAL(list_elementExists(unsortedSet, 1), false);
		ASSERT_EQUAL(list_elementExists(unsortedSet, 2), true);
		ASSERT_EQUAL(list_elementExists(unsortedSet, 3), false);
		ASSERT_EQUAL(list_elementExists(unsortedSet, 4), true);
		ASSERT_EQUAL(list_elementExists(unsortedSet, 5), true);
		ASSERT_EQUAL(list_elementExists(unsortedSet, 6), false);
		ASSERT_EQUAL(list_elementExists(unsortedSet, 7), true);
		ASSERT_EQUAL(list_elementExists(unsortedSet, 8), false);
		// New value.
		ASSERT_EQUAL(list_insertUnique_last(unsortedSet, 3), true)
		ASSERT_EQUAL(unsortedSet, List<int32_t>(7, 5, 2, 4, 3));
		// Already exists.
		ASSERT_EQUAL(list_insertUnique_last(unsortedSet, 5), false)
		ASSERT_EQUAL(unsortedSet, List<int32_t>(7, 5, 2, 4, 3));
		// New value.
		ASSERT_EQUAL(list_insertUnique_last(unsortedSet, 6), true)
		ASSERT_EQUAL(unsortedSet, List<int32_t>(7, 5, 2, 4, 3, 6));
	}
	{
		List<int32_t> unsortedUnion(7, 5, 2, 4);
		// Nothing is inserted, because all inserted elements already exist.
		ASSERT_EQUAL(list_insertUnion_last(unsortedUnion, List<int32_t>(5, 2)), false);
		ASSERT_EQUAL(unsortedUnion, List<int32_t>(7, 5, 2, 4));
		// Unique values (3 and 6) are inserted at the end.
		ASSERT_EQUAL(list_insertUnion_last(unsortedUnion, List<int32_t>(3, 5, 6)), true);
		ASSERT_EQUAL(unsortedUnion, List<int32_t>(7, 5, 2, 4, 3, 6));
	}
	{
		List<int32_t> sortedSet(2, 4, 5, 7);
		ASSERT_EQUAL(list_elementExists(sortedSet, 1), false);
		ASSERT_EQUAL(list_elementExists(sortedSet, 2), true);
		ASSERT_EQUAL(list_elementExists(sortedSet, 3), false);
		ASSERT_EQUAL(list_elementExists(sortedSet, 4), true);
		ASSERT_EQUAL(list_elementExists(sortedSet, 5), true);
		ASSERT_EQUAL(list_elementExists(sortedSet, 6), false);
		ASSERT_EQUAL(list_elementExists(sortedSet, 7), true);
		ASSERT_EQUAL(list_elementExists(sortedSet, 8), false);
		// New value.
		ASSERT_EQUAL(list_insertUnique_sorted_ascending(sortedSet, 3), true)
		ASSERT_EQUAL(sortedSet, List<int32_t>(2, 3, 4, 5, 7));
		// Already exists.
		ASSERT_EQUAL(list_insertUnique_sorted_ascending(sortedSet, 5), false)
		ASSERT_EQUAL(sortedSet, List<int32_t>(2, 3, 4, 5, 7));
		// New value.
		ASSERT_EQUAL(list_insertUnique_sorted_ascending(sortedSet, 6), true)
		ASSERT_EQUAL(sortedSet, List<int32_t>(2, 3, 4, 5, 6, 7));
	}
	{ // Sorted unions, which are useful for comparing if two sets contain the same values.
		List<int32_t> sortedUnion(2, 4, 5, 7);
		// Nothing is inserted, because all inserted elements already exist.
		ASSERT_EQUAL(list_insertUnion_sorted_ascending(sortedUnion, List<int32_t>(5, 2)), false);
		ASSERT_EQUAL(sortedUnion, List<int32_t>(2, 4, 5, 7));
		// Unique values (3 and 6) are inserted in ascending order.
		ASSERT_EQUAL(list_insertUnion_sorted_ascending(sortedUnion, List<int32_t>(3, 5, 6)), true);
		ASSERT_EQUAL(sortedUnion, List<int32_t>(2, 3, 4, 5, 6, 7));
	}
	// TODO: Test with custom comparison functions.
	// TODO: Implement bruteforce testing.
END_TEST
