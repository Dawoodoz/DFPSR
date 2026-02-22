
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
		List<intptr_t> alivePersonIndices = list_findAll<Person>(people, [](const Person &person) -> bool { return person.alive; });
		ASSERT_EQUAL(alivePersonIndices, List<intptr_t>(3, 4, 6));
		// Find all dead.
		List<intptr_t> deadPersonIndices = list_findAll<Person>(people, [](const Person &person) -> bool { return !(person.alive); });
		ASSERT_EQUAL(deadPersonIndices, List<intptr_t>(0, 1, 2, 5));
		// Find first alive.
		ASSERT_EQUAL(list_findFirst<Person>(people, [](const Person &person) -> bool { return person.alive; }), 3);
		// Find first dead.
		ASSERT_EQUAL(list_findFirst<Person>(people, [](const Person &person) -> bool { return !(person.alive); }), 0);
		// Find last alive.
		ASSERT_EQUAL(list_findLast<Person>(people, [](const Person &person) -> bool { return person.alive; }), 6);
		// Find last alive.
		ASSERT_EQUAL(list_findLast<Person>(people, [](const Person &person) -> bool { return !(person.alive); }), 5);
	}
	// TODO: Write more tests.
	// * list_elementExists
	// * list_elementIsMissing
	// * list_insert_last
	// * list_insert_sorted_ascending
	// * list_insertUnique_last
	// * list_insertUnique_sorted_ascending
	// * list_append_last
	// * list_append_sorted_ascending
	// * list_appendUnique_last
	// * list_appendUnique_sorted_ascending

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
		ASSERT_EQUAL(list_appendUnique_last(unsortedUnion, List<int32_t>(5, 2)), false);
		ASSERT_EQUAL(unsortedUnion, List<int32_t>(7, 5, 2, 4));
		// Unique values (3 and 6) are inserted at the end.
		ASSERT_EQUAL(list_appendUnique_last(unsortedUnion, List<int32_t>(3, 5, 6)), true);
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
		ASSERT_EQUAL(list_appendUnique_sorted_ascending(sortedUnion, List<int32_t>(5, 2)), false);
		ASSERT_EQUAL(sortedUnion, List<int32_t>(2, 4, 5, 7));
		// Unique values (3 and 6) are inserted in ascending order.
		ASSERT_EQUAL(list_appendUnique_sorted_ascending(sortedUnion, List<int32_t>(3, 5, 6)), true);
		ASSERT_EQUAL(sortedUnion, List<int32_t>(2, 3, 4, 5, 6, 7));
	}
	// TODO: Test with custom comparison functions.
	// TODO: Implement bruteforce testing.
END_TEST
