
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
static_assert(!std::is_copy_constructible<Unique>::value, "The Unique type should not be copy constructible!");
static_assert(!std::is_default_constructible<Unique>::value, "The Unique type should not be default constructible!");

struct Tree {
	String name;
	List<Tree> children;
	Tree() {}
	Tree(const ReadableString &name)
	: name(name) {}
	Tree(const ReadableString &name, List<Tree> children)
	: name(name), children(children) {}
	// Copy/move construct/assign should be generated automatically by the compiler, because only constructors have been specified manually.
};
static_assert(std::is_move_constructible<Tree>::value, "The Tree type should be move constructible!");
static_assert(std::is_copy_constructible<Tree>::value, "The Tree type should be copy constructible!");
static_assert(std::is_default_constructible<Tree>::value, "The Tree type should be default constructible!");

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
		// Move the whole list.
		List<Unique> objects2 = std::move(objects);
		ASSERT_EQUAL(objects.length(), 0);
		ASSERT_EQUAL(objects2.length(), 3);
		ASSERT_EQUAL(objects2[0].name, U"Two");
		ASSERT_EQUAL(objects2[1].name, U"One");
		ASSERT_EQUAL(objects2[2].name, U"Three");
	}
	{
		// Default movable and copyable types should clone the content recursively when the list is copied.
		Tree treeOne = Tree(U"A", List<Tree>(Tree(U"B", List<Tree>(Tree(U"D"), Tree(U"E"))), Tree(U"C")));
		ASSERT_EQUAL(treeOne.name, U"A");
		ASSERT_EQUAL(treeOne.children.length(), 2);
			ASSERT_EQUAL(treeOne.children[0].name, U"B");
			ASSERT_EQUAL(treeOne.children[0].children.length(), 2);
				ASSERT_EQUAL(treeOne.children[0].children[0].name, U"D");
				ASSERT_EQUAL(treeOne.children[0].children[0].children.length(), 0);
				ASSERT_EQUAL(treeOne.children[0].children[1].name, U"E");
				ASSERT_EQUAL(treeOne.children[0].children[1].children.length(), 0);
			ASSERT_EQUAL(treeOne.children[1].name, U"C");
			ASSERT_EQUAL(treeOne.children[1].children.length(), 0);
		// Copy the tree using copy assignment. (Default construction can be optimized away.)
		Tree treeTwo = treeOne;
		// Modify an element in treeTwo to have the content copied to the next trees.
		treeTwo.children[0].name = U"BBBB";
		// Copy the tree using copy construction. (Default construction can be optimized away.)
		Tree treeThree = Tree(treeTwo);
		// Copy the tree using default generated copy construction. (Direct construction without any assignment.)
		Tree treeFour(treeThree);
		// Modify each version of the tree
		treeOne.name   = U"A1";
		treeTwo.name   = U"A2";
		treeThree.name = U"A3";
		treeFour.name  = U"A4";
		ASSERT_EQUAL(treeOne.name  , U"A1");
		ASSERT_EQUAL(treeOne.children.length(), 2);
		ASSERT_EQUAL(treeTwo.name  , U"A2");
		ASSERT_EQUAL(treeTwo.children.length(), 2);
		ASSERT_EQUAL(treeThree.name, U"A3");
		ASSERT_EQUAL(treeThree.children.length(), 2);
		ASSERT_EQUAL(treeFour.name , U"A4");
		ASSERT_EQUAL(treeFour.children.length(), 2);
			ASSERT_EQUAL(treeFour.children[0].name, U"BBBB");
			ASSERT_EQUAL(treeFour.children[0].children.length(), 2);
				ASSERT_EQUAL(treeFour.children[0].children[0].name, U"D");
				ASSERT_EQUAL(treeFour.children[0].children[0].children.length(), 0);
				ASSERT_EQUAL(treeFour.children[0].children[1].name, U"E");
				ASSERT_EQUAL(treeFour.children[0].children[1].children.length(), 0);
			ASSERT_EQUAL(treeFour.children[1].name, U"C");
			ASSERT_EQUAL(treeFour.children[1].children.length(), 0);
		// Move the first tree to a new location.
		Tree newTree = std::move(treeOne);
		ASSERT_EQUAL(treeOne.children.length(), 0);
		ASSERT_EQUAL(newTree.name, U"A1");
		ASSERT_EQUAL(newTree.children.length(), 2);
			ASSERT_EQUAL(newTree.children[0].name, U"B");
			ASSERT_EQUAL(newTree.children[0].children.length(), 2);
				ASSERT_EQUAL(newTree.children[0].children[0].name, U"D");
				ASSERT_EQUAL(newTree.children[0].children[0].children.length(), 0);
				ASSERT_EQUAL(newTree.children[0].children[1].name, U"E");
				ASSERT_EQUAL(newTree.children[0].children[1].children.length(), 0);
			ASSERT_EQUAL(newTree.children[1].name, U"C");
			ASSERT_EQUAL(newTree.children[1].children.length(), 0);
	}
	{
		// Construct and push.
		Tree tree = Tree(U"A");
		tree.children.push(Tree(U"B", List<Tree>(Tree(U"D"), Tree(U"E"))));
		tree.children.push(Tree(U"C"));
		ASSERT_EQUAL(tree.name, U"A");
		ASSERT_EQUAL(tree.children.length(), 2);
			ASSERT_EQUAL(tree.children[0].name, U"B");
			ASSERT_EQUAL(tree.children[0].children.length(), 2);
				ASSERT_EQUAL(tree.children[0].children[0].name, U"D");
				ASSERT_EQUAL(tree.children[0].children[0].children.length(), 0);
				ASSERT_EQUAL(tree.children[0].children[1].name, U"E");
				ASSERT_EQUAL(tree.children[0].children[1].children.length(), 0);
			ASSERT_EQUAL(tree.children[1].name, U"C");
			ASSERT_EQUAL(tree.children[1].children.length(), 0);
	}
	{
		// Push-construct.
		Tree tree = Tree(U"A");
		tree.children.pushConstruct(U"B", List<Tree>(Tree(U"D"), Tree(U"E")));
		tree.children.pushConstruct(U"C");
		ASSERT_EQUAL(tree.name, U"A");
		ASSERT_EQUAL(tree.children.length(), 2);
			ASSERT_EQUAL(tree.children[0].name, U"B");
			ASSERT_EQUAL(tree.children[0].children.length(), 2);
				ASSERT_EQUAL(tree.children[0].children[0].name, U"D");
				ASSERT_EQUAL(tree.children[0].children[0].children.length(), 0);
				ASSERT_EQUAL(tree.children[0].children[1].name, U"E");
				ASSERT_EQUAL(tree.children[0].children[1].children.length(), 0);
			ASSERT_EQUAL(tree.children[1].name, U"C");
			ASSERT_EQUAL(tree.children[1].children.length(), 0);
	}
END_TEST
