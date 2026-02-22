
#include "../testTools.h"
#include "../../DFPSR/api/algorithmAPI_List.h"

int32_t doubleValue(const int32_t &element) {
	return element * 2;
}

struct Person {
	int32_t birthYear;
	bool alive;
	Person(int32_t birthYear, bool alive)
	: birthYear(birthYear), alive(alive) {}
};

START_TEST(ListAlgorithm)
	{ // Map to and from the same type using function pointer.
		// Using the map function instead of pushing from a loop will only have to allocate the output and ask if the function has a closure once for all calls.
		List<int32_t> input(1, 2, 3, 4, 5);
		List<int32_t> output = list_map<int32_t, int32_t>(input, &doubleValue);
		ASSERT_EQUAL(output, List<int32_t>(2, 4, 6, 8, 10));
		input.clear();
		output = list_map<int32_t, int32_t>(input, &doubleValue);
		ASSERT_EQUAL(output, List<int32_t>());
	}
	{ // Map to and from the same type using lambda.
		List<int32_t> input(1, 2, 3, 4, 5);
		List<int32_t> output = list_map<int32_t, int32_t>(input, [](const int32_t &element) -> int32_t {
			return element * 2;
		});
		ASSERT_EQUAL(output, List<int32_t>(2, 4, 6, 8, 10));
	}
	{ // Map to different type using lambda.
		List<int32_t> input(8, 5, 7, 3);
		List<String> output = list_map<String, int32_t>(input, [](const int32_t &element) -> String {
			return string_combine(element + 1);
		});
		ASSERT_EQUAL(output, List<String>(U"9", U"6", U"8", U"4"));
	}
	{ // Find elements in lists.
		List<int32_t> values(4, 1, 3, 7, 34, 72, 2, 45, 6, 4, 7, 7, 2);
		// Check if 7 exists.
		ASSERT_EQUAL(list_elementExists(values, 7), true);
		ASSERT_EQUAL(list_elementIsMissing(values, 7), false);
		// Find the first 7 at index 3 in values.
		ASSERT_EQUAL(list_findFirst(values, 7), 3);
		// Find the last 7 at index 11 in values.
		ASSERT_EQUAL(list_findLast(values, 7), 11);
		// Find the last 7 at indices 3, 10 and 11 in values.
		ASSERT_EQUAL(list_findAll(values, 7), List<intptr_t>(3, 10, 11));
		// Value 7 does not exist in an empty list.
		ASSERT_EQUAL(list_elementExists(List<int32_t>(), 7), false);
		ASSERT_EQUAL(list_elementIsMissing(List<int32_t>(), 7), true);
		ASSERT_EQUAL(list_findFirst(List<int32_t>(), 7), -1);
		ASSERT_EQUAL(list_findLast(List<int32_t>(), 7), -1);
		ASSERT_EQUAL(list_findAll(List<int32_t>(), 7), List<intptr_t>());
		// Value 8 does not exist in the list.
		ASSERT_EQUAL(list_elementExists(values, 8), false);
		ASSERT_EQUAL(list_elementIsMissing(values, 8), true);
		ASSERT_EQUAL(list_findFirst(values, 8), -1);
		ASSERT_EQUAL(list_findLast(values, 8), -1);
		ASSERT_EQUAL(list_findAll(values, 8), List<intptr_t>());
	}
	{ // Find using custom criteria.
		List<Person> people(
			Person(1584, false), // 0
			Person(1714, false), // 1
			Person(1825, false), // 2
			Person(1983, true),  // 3
			Person(1990, true),  // 4
			Person(2009, false), // 5
			Person(2012, true)   // 6
		);
		// Find all alive.
		List<intptr_t> alivePersonIndices = list_findAll<Person>(people, [](intptr_t index, const Person &person) -> bool { return person.alive; });
		ASSERT_EQUAL(alivePersonIndices, List<intptr_t>(3, 4, 6));
		// Find all dead.
		List<intptr_t> deadPersonIndices = list_findAll<Person>(people, [](intptr_t index, const Person &person) -> bool { return !(person.alive); });
		ASSERT_EQUAL(deadPersonIndices, List<intptr_t>(0, 1, 2, 5));
		// Find first alive.
		ASSERT_EQUAL(list_findFirst<Person>(people, [](intptr_t index, const Person &person) -> bool { return person.alive; }), 3);
		// Find first dead.
		ASSERT_EQUAL(list_findFirst<Person>(people, [](intptr_t index, const Person &person) -> bool { return !(person.alive); }), 0);
		// Find last alive.
		ASSERT_EQUAL(list_findLast<Person>(people, [](intptr_t index, const Person &person) -> bool { return person.alive; }), 6);
		// Find last alive.
		ASSERT_EQUAL(list_findLast<Person>(people, [](intptr_t index, const Person &person) -> bool { return !(person.alive); }), 5);
	}
	{ // Checking if a list is sorted.
		ASSERT_EQUAL(list_isSorted_ascending(List<int32_t>()), true);
		ASSERT_EQUAL(list_isSorted_ascending(List<int32_t>(1)), true);
		ASSERT_EQUAL(list_isSorted_ascending(List<int32_t>(1, 2)), true);
		ASSERT_EQUAL(list_isSorted_ascending(List<int32_t>(1, 2, 3)), true);
		ASSERT_EQUAL(list_isSorted_ascending(List<int32_t>(1, 2, 3, 4)), true);
		ASSERT_EQUAL(list_isSorted_ascending(List<int32_t>(1, 3, 2, 4)), false);
		ASSERT_EQUAL(list_isSorted_ascending(List<int32_t>(4, 3, 2, 1)), false);
		ASSERT_EQUAL(list_isSorted_ascending(List<int32_t>(1, 1, 2, 2, 3, 3, 4, 4)), true);
		ASSERT_EQUAL(list_isSorted_descending(List<int32_t>(4, 4, 3, 3, 2, 2, 1, 1)), true);
		ASSERT_EQUAL(list_isSorted_descending(List<int32_t>(1, 2, 3, 4)), false);
		ASSERT_EQUAL(list_isSorted_descending(List<int32_t>(1, 3, 2, 4)), false);
		ASSERT_EQUAL(list_isSorted_descending(List<int32_t>(4, 3, 2, 1)), true);
		ASSERT_EQUAL(list_isSorted_descending(List<int32_t>(4, 3, 2)), true);
		ASSERT_EQUAL(list_isSorted_descending(List<int32_t>(3, 2, 1)), true);
		ASSERT_EQUAL(list_isSorted_descending(List<int32_t>(3, 2)), true);
		ASSERT_EQUAL(list_isSorted_descending(List<int32_t>(2)), true);
		ASSERT_EQUAL(list_isSorted_descending(List<int32_t>()), true);
	}
	{ // Insert elements into ascending list.
		List<int32_t> ascendingList(-4, 2, 6, 12, 64, 236);
		ASSERT_EQUAL(list_insert_sorted_ascending(ascendingList, 1), 1); // Inserted at index 1.
		//                                            _
		ASSERT_EQUAL(ascendingList, List<int32_t>(-4, 1, 2, 6, 12, 64, 236));
		ASSERT_EQUAL(list_insert_sorted_ascending(ascendingList, -63), 0); // Inserted at index 0.
		//                                        ___
		ASSERT_EQUAL(ascendingList, List<int32_t>(-63, -4, 1, 2, 6, 12, 64, 236));
		ASSERT_EQUAL(list_insert_sorted_ascending(ascendingList, 236), 8); // Inserted at index 8.
		//                                                                       ___
		ASSERT_EQUAL(ascendingList, List<int32_t>(-63, -4, 1, 2, 6, 12, 64, 236, 236));
		ASSERT_EQUAL(list_insert_sorted_ascending(ascendingList, 634), 9); // Inserted at index 9.
		//                                                                            ___
		ASSERT_EQUAL(ascendingList, List<int32_t>(-63, -4, 1, 2, 6, 12, 64, 236, 236, 634));
		ASSERT_EQUAL(list_insert_sorted_ascending(ascendingList, 54), 6); // Inserted at index 6.
		//                                                              __
		ASSERT_EQUAL(ascendingList, List<int32_t>(-63, -4, 1, 2, 6, 12, 54, 64, 236, 236, 634));
	}
	{ // Insert elements into ascending list.
		List<int32_t> ascendingList(-4, 2, 6, 12, 64, 236);
		list_append_sorted_ascending(ascendingList, List<int32_t>(1, -63, 236, 634, 54));
		ASSERT_EQUAL(ascendingList, List<int32_t>(-63, -4, 1, 2, 6, 12, 54, 64, 236, 236, 634));
	}
	{ // Insert elements into descending list.
		List<int32_t> descendingList(236, 64, 12, 6, 2, -4);
		ASSERT_EQUAL(list_insert_sorted_descending(descendingList, 1), 5);
		//                                                            _
		ASSERT_EQUAL(descendingList, List<int32_t>(236, 64, 12, 6, 2, 1, -4));
		ASSERT_EQUAL(list_insert_sorted_descending(descendingList, -63), 7);
		//                                                                   ___
		ASSERT_EQUAL(descendingList, List<int32_t>(236, 64, 12, 6, 2, 1, -4, -63));
		ASSERT_EQUAL(list_insert_sorted_descending(descendingList, 236), 1);
		//                                              ___
		ASSERT_EQUAL(descendingList, List<int32_t>(236, 236, 64, 12, 6, 2, 1, -4, -63));
		ASSERT_EQUAL(list_insert_sorted_descending(descendingList, 634), 0);
		//                                         ___
		ASSERT_EQUAL(descendingList, List<int32_t>(634, 236, 236, 64, 12, 6, 2, 1, -4, -63));
		ASSERT_EQUAL(list_insert_sorted_descending(descendingList, 54), 4);
		//                                                            __
		ASSERT_EQUAL(descendingList, List<int32_t>(634, 236, 236, 64, 54, 12, 6, 2, 1, -4, -63));
	}
	{ // Insert elements into descending list.
		List<int32_t> descendingList(236, 64, 12, 6, 2, -4);
		list_append_sorted_descending(descendingList, List<int32_t>(1, -63, 236, 634, 54));
		ASSERT_EQUAL(descendingList, List<int32_t>(634, 236, 236, 64, 54, 12, 6, 2, 1, -4, -63));
	}
	{ // Insert elements into unsorted list.
		List<int32_t> unsortedList(5, 2, 4, 7, 3, 6);
		list_insert_last(unsortedList, 4);
		ASSERT_EQUAL(unsortedList, List<int32_t>(5, 2, 4, 7, 3, 6, 4));
		list_insert_last(unsortedList, 1);
		ASSERT_EQUAL(unsortedList, List<int32_t>(5, 2, 4, 7, 3, 6, 4, 1));
	}
	{ // Apend elements into unsorted list.
		List<int32_t> unsortedList(5, 2, 4, 7, 3, 6);
		list_append_last(unsortedList, List<int32_t>(13, 35, 23, 67, 44));
		ASSERT_EQUAL(unsortedList, List<int32_t>(5, 2, 4, 7, 3, 6, 13, 35, 23, 67, 44));
	}
	// TODO: Write more tests.
	// * list_insert_unique_last
	// * list_insert_unique_sorted_ascending
	// * list_insert_unique_sorted_descending
	// * list_append_unique_last
	// * list_append_unique_sorted
	// * list_append_unique_sorted_ascending
	// * list_append_unique_sorted_descending

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
		ASSERT_EQUAL(list_insert_unique_last(unsortedSet, 3), 4)
		ASSERT_EQUAL(unsortedSet, List<int32_t>(7, 5, 2, 4, 3));
		// Already exists.
		ASSERT_EQUAL(list_insert_unique_last(unsortedSet, 5), -1)
		ASSERT_EQUAL(unsortedSet, List<int32_t>(7, 5, 2, 4, 3));
		// New value.
		ASSERT_EQUAL(list_insert_unique_last(unsortedSet, 6), 5)
		ASSERT_EQUAL(unsortedSet, List<int32_t>(7, 5, 2, 4, 3, 6));
	}
	{
		List<int32_t> unsortedUnion(7, 5, 2, 4);
		// Nothing is inserted, because all inserted elements already exist.
		ASSERT_EQUAL(list_append_unique_last(unsortedUnion, List<int32_t>(5, 2)), false);
		ASSERT_EQUAL(unsortedUnion, List<int32_t>(7, 5, 2, 4));
		// Unique values (3 and 6) are inserted at the end.
		ASSERT_EQUAL(list_append_unique_last(unsortedUnion, List<int32_t>(3, 5, 6)), true);
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
		ASSERT_EQUAL(list_insert_unique_sorted_ascending(sortedSet, 3), 1)
		ASSERT_EQUAL(sortedSet, List<int32_t>(2, 3, 4, 5, 7));
		// Already exists.
		ASSERT_EQUAL(list_insert_unique_sorted_ascending(sortedSet, 5), -1)
		ASSERT_EQUAL(sortedSet, List<int32_t>(2, 3, 4, 5, 7));
		// New value.
		ASSERT_EQUAL(list_insert_unique_sorted_ascending(sortedSet, 6), 4)
		ASSERT_EQUAL(sortedSet, List<int32_t>(2, 3, 4, 5, 6, 7));
	}
	{ // Sorted unions, which are useful for comparing if two sets contain the same values.
		List<int32_t> sortedUnion(2, 4, 5, 7);
		// Nothing is inserted, because all inserted elements already exist.
		ASSERT_EQUAL(list_append_unique_sorted_ascending(sortedUnion, List<int32_t>(5, 2)), false);
		ASSERT_EQUAL(sortedUnion, List<int32_t>(2, 4, 5, 7));
		// Unique values (3 and 6) are inserted in ascending order.
		ASSERT_EQUAL(list_append_unique_sorted_ascending(sortedUnion, List<int32_t>(3, 5, 6)), true);
		ASSERT_EQUAL(sortedUnion, List<int32_t>(2, 3, 4, 5, 6, 7));
	}
	// TODO: Test with custom comparison functions.
	// TODO: Implement bruteforce testing.
END_TEST
