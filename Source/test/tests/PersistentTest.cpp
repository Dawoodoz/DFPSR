
#include "../testTools.h"
#include "../../DFPSR/persistent/includePersistent.h"

// -------- Test example below --------

class MyClass : public Persistent {
PERSISTENT_DECLARATION(MyClass)
public:
	PersistentInteger a;
	PersistentString b;
public:
	MyClass() {}
	MyClass(int a, const String &b) : a(a), b(PersistentString::unmangled(b)) {}
public:
	void declareAttributes(StructureDefinition &target) const override {
		target.declareAttribute(U"a");
		target.declareAttribute(U"b");
	}
	Persistent* findAttribute(const ReadableString &name) override {
		if (string_caseInsensitiveMatch(name, U"a")) {
			return &(this->a);
		} else if (string_caseInsensitiveMatch(name, U"b")) {
			return &(this->b);
		} else {
			return nullptr;
		}
	}
};
PERSISTENT_DEFINITION(MyClass)

class MySubClass : public MyClass {
PERSISTENT_DECLARATION(MySubClass)
public:
	PersistentInteger c;
	PersistentInteger d;
public:
	MySubClass() {}
	MySubClass(int a, const String &b, int c, int d) : MyClass(a, b), c(c), d(d) {}
public:
	void declareAttributes(StructureDefinition &target) const override {
		MyClass::declareAttributes(target);
		target.declareAttribute(U"c");
		target.declareAttribute(U"d");
	}
	Persistent* findAttribute(const ReadableString &name) override {
		if (string_caseInsensitiveMatch(name, U"c")) {
			return &(this->c);
		} else if (string_caseInsensitiveMatch(name, U"d")) {
			return &(this->d);
		} else {
			return MyClass::findAttribute(name);
		}
	}
};
PERSISTENT_DEFINITION(MySubClass)

class MyCollection : public MyClass {
PERSISTENT_DECLARATION(MyCollection)
public:
	MyCollection() {}
	MyCollection(int a, const String &b) : MyClass(a, b) {}
public:
	List<std::shared_ptr<MyClass>> children;
	bool addChild(std::shared_ptr<Persistent> child) override {
		// Try to cast from base class Persistent to derived class MyClass
		std::shared_ptr<MyClass> myClass = std::dynamic_pointer_cast<MyClass>(child);
		if (myClass.get() == nullptr) {
			return false; // Wrong type!
		} else {
			this->children.push(myClass);
			return true; // Success!
		}
	}
	int getChildCount() const override {
		return this->children.length();
	}
	std::shared_ptr<Persistent> getChild(int index) const override {
		return this->children[index];
	}
};
PERSISTENT_DEFINITION(MyCollection)

String exampleOne =
R"QUOTE(Begin : MyClass
	a = 1
	b = "two"
End
)QUOTE";

String exampleTwo =
R"QUOTE(Begin : MySubClass
	a = 1
	b = "two"
	c = 3
	d = 4
End
)QUOTE";

String exampleThree =
R"QUOTE(Begin : MyCollection
	a = 1
	b = "first"
	Begin : MyClass
		a = 12
		b = "test"
	End
	Begin : MyCollection
		a = 2
		b = "second"
		Begin : MyClass
			a = 3
			b = "third"
		End
	End
	Begin : MySubClass
		a = 34
		b = "foo"
		c = 56
		d = 78
	End
End
)QUOTE";

START_TEST(Persistent)
	// Register the non-atomic classes
	REGISTER_PERSISTENT_CLASS(MyClass)
	REGISTER_PERSISTENT_CLASS(MySubClass)
	REGISTER_PERSISTENT_CLASS(MyCollection)

	// MyClass to text
	MyClass myObject(1, U"two");
	String myText = myObject.toString();
	ASSERT_MATCH(myText, exampleOne);

	// MyClass from text
	std::shared_ptr<Persistent> myObjectCopy = createPersistentClassFromText(myText, U"");
	ASSERT_MATCH(myObjectCopy->toString(), myText);

	// MySubClass to text
	MySubClass mySubObject(1, U"two", 3, 4);
	String MySubText = mySubObject.toString();
	ASSERT_MATCH(MySubText, exampleTwo);

	// MySubClass from text
	std::shared_ptr<Persistent> mySubObjectCopy = createPersistentClassFromText(MySubText, U"");
	ASSERT_MATCH(mySubObjectCopy->toString(), MySubText);

	// Tree structure to text
	MyCollection tree(1, U"first");
	std::shared_ptr<MyCollection> second = std::make_shared<MyCollection>(2, U"second");
	tree.addChild(std::make_shared<MyClass>(12, U"test"));
	tree.addChild(second);
	tree.addChild(std::make_shared<MySubClass>(34, U"foo", 56, 78));
	second->addChild(std::make_shared<MyClass>(3, U"third"));
	ASSERT_MATCH(tree.toString(), exampleThree);

	// Tree structure from text
	std::shared_ptr<Persistent> treeCopy = createPersistentClassFromText(exampleThree, U"");
	ASSERT_MATCH(treeCopy->toString(), exampleThree);

	// Persistent string lists
	PersistentStringList myList = PersistentStringList();
	ASSERT_EQUAL(myList.value.length(), 0);
	ASSERT_MATCH(myList.toString(), U"");

	myList = PersistentStringList(U"\"\"", U"");
	ASSERT_EQUAL(myList.value.length(), 1);
	ASSERT_MATCH(myList.value[0], U"");
	ASSERT_MATCH(myList.toString(), U"\"\"");

	myList = PersistentStringList(U"\"A\", \"B\"", U"");
	ASSERT_EQUAL(myList.value.length(), 2);
	ASSERT_MATCH(myList.value[0], U"A");
	ASSERT_MATCH(myList.value[1], U"B");
	ASSERT_MATCH(myList.toString(), U"\"A\", \"B\"");

	myList.assignValue(U"\"Only element\"", U"");
	ASSERT_EQUAL(myList.value.length(), 1);
	ASSERT_MATCH(myList.value[0], U"Only element");
	ASSERT_MATCH(myList.toString(), U"\"Only element\"");

	myList = PersistentStringList(U"", U"");
	ASSERT_EQUAL(myList.value.length(), 0);
	ASSERT_MATCH(myList.toString(), U"");

	myList.assignValue(U"\"Zero 0\", \"One 1\", \"Two 2\", \"Three 3\"", U"");
	ASSERT_EQUAL(myList.value.length(), 4);
	ASSERT_MATCH(myList.value[0], U"Zero 0");
	ASSERT_MATCH(myList.value[1], U"One 1");
	ASSERT_MATCH(myList.value[2], U"Two 2");
	ASSERT_MATCH(myList.value[3], U"Three 3");
	ASSERT_MATCH(myList.toString(), U"\"Zero 0\", \"One 1\", \"Two 2\", \"Three 3\"");
END_TEST
