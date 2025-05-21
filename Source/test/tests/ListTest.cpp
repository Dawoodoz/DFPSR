
#include "../testTools.h"
#include "../../DFPSR/collection/List.h"

static void targetByReference(List<int32_t> &target, int32_t value) {
	target.push(value);
}

struct Unique {
	String name;
	// Constructible
	Unique(const ReadableString &name) : name(name) {}
	// Movable
	Unique(Unique &&original) = default;
	Unique& operator=(Unique &&original) = default;
	// Destructible
	~Unique() = default;
	// Not clonable
	Unique(const Unique& original) = delete;
	Unique& operator=(const Unique& original) = delete;
};
static_assert(std::is_move_constructible<Unique>::value, "The Unique type should be move constructible!");

START_TEST(List)
	{
		// Populate
		List<int32_t> integers;
		ASSERT_EQUAL(integers.length(), 0);
		targetByReference(integers, 5);
		ASSERT_EQUAL(integers.length(), 1);
		ASSERT_EQUAL(integers[0], 5);
		targetByReference(integers, 86);
		ASSERT_EQUAL(integers.length(), 2);
		ASSERT_EQUAL(integers[0], 5);
		ASSERT_EQUAL(integers[1], 86);
		std::function<void(int32_t value)> method = [&integers](int32_t value) {
			integers.push(value);
		};
		method(24);
		ASSERT_EQUAL(integers.length(), 3);
		ASSERT_EQUAL(integers[0], 5);
		ASSERT_EQUAL(integers[1], 86);
		ASSERT_EQUAL(integers[2], 24);
		integers.pushConstruct(123);
		ASSERT_EQUAL(integers.length(), 4);
		ASSERT_EQUAL(integers[0], 5);
		ASSERT_EQUAL(integers[1], 86);
		ASSERT_EQUAL(integers[2], 24);
		ASSERT_EQUAL(integers[3], 123);
		// Copy
		List<int32_t> copied = List<int32_t>(integers);
		ASSERT_EQUAL(integers.length(), 4);
		ASSERT_EQUAL(integers[0], 5);
		ASSERT_EQUAL(integers[1], 86);
		ASSERT_EQUAL(integers[2], 24);
		ASSERT_EQUAL(integers[3], 123);
		ASSERT_EQUAL(copied.length(), 4);
		ASSERT_EQUAL(copied[0], 5);
		ASSERT_EQUAL(copied[1], 86);
		ASSERT_EQUAL(copied[2], 24);
		ASSERT_EQUAL(copied[3], 123);
		// Assign
		List<int32_t> assigned = integers;
		ASSERT_EQUAL(integers.length(), 4);
		ASSERT_EQUAL(integers[0], 5);
		ASSERT_EQUAL(integers[1], 86);
		ASSERT_EQUAL(integers[2], 24);
		ASSERT_EQUAL(integers[3], 123);
		ASSERT_EQUAL(assigned.length(), 4);
		ASSERT_EQUAL(assigned[0], 5);
		ASSERT_EQUAL(assigned[1], 86);
		ASSERT_EQUAL(assigned[2], 24);
		ASSERT_EQUAL(assigned[3], 123);
		// Move
		List<int32_t> moved = std::move(integers);
		ASSERT_EQUAL(integers.length(), 0);
		ASSERT_EQUAL(moved.length(), 4);
		ASSERT_EQUAL(moved[0], 5);
		ASSERT_EQUAL(moved[1], 86);
		ASSERT_EQUAL(moved[2], 24);
		ASSERT_EQUAL(moved[3], 123);
	}
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
	{
		// Non-copyable types ensure that the constructed object is not accidentally copied into another location.
		List<Unique> objects = List<Unique>(Unique(U"One"), Unique(U"Two"));
		ASSERT_EQUAL(objects.length(), 2);
		ASSERT_EQUAL(objects[0].name, U"One");
		ASSERT_EQUAL(objects[1].name, U"Two");
		// The push method can not be called with on non-copyable element types, because push must copy the given element.
		//objects.push(Unique(U"Three"));
		// The pushConstruct method can be used instead to construct the element inside of the list.
		objects.pushConstruct(U"Three");
		ASSERT_EQUAL(objects.length(), 3);
		ASSERT_EQUAL(objects[0].name, U"One");
		ASSERT_EQUAL(objects[1].name, U"Two");
		ASSERT_EQUAL(objects[2].name, U"Three");
		objects.swap(0, 1);
		ASSERT_EQUAL(objects.length(), 3);
		ASSERT_EQUAL(objects[0].name, U"Two");
		ASSERT_EQUAL(objects[1].name, U"One");
		ASSERT_EQUAL(objects[2].name, U"Three");
	}
END_TEST
