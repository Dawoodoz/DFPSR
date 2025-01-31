
#include "../testTools.h"
#include "../../DFPSR/implementation/persistent/includePersistent.h"

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
	List<Handle<MyClass>> children;
	bool addChild(Handle<Persistent> child) override {
		// Try to cast from base class Persistent to derived class MyClass
		Handle<MyClass> myClass = handle_dynamicCast<MyClass>(child);
		if (myClass.isNull()) {
			return false; // Wrong type!
		} else {
			this->children.push(myClass);
			return true; // Success!
		}
	}
	int getChildCount() const override {
		return this->children.length();
	}
	Handle<Persistent> getChild(int index) const override {
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
	ASSERT_EQUAL(myText, exampleOne);

	// MyClass from text
	Handle<Persistent> myObjectCopy = createPersistentClassFromText(myText, U"");
	ASSERT_EQUAL(myObjectCopy->toString(), myText);

	// MySubClass to text
	MySubClass mySubObject(1, U"two", 3, 4);
	String MySubText = mySubObject.toString();
	ASSERT_EQUAL(MySubText, exampleTwo);

	// MySubClass from text
	Handle<Persistent> mySubObjectCopy = createPersistentClassFromText(MySubText, U"");
	ASSERT_EQUAL(mySubObjectCopy->toString(), MySubText);

	// Tree structure to text
	MyCollection tree(1, U"first");
	ASSERT_EQUAL(tree.getChildCount(), 0);
	Handle<MyCollection> second = handle_create<MyCollection>(2, U"second");
	tree.addChild(handle_create<MyClass>(12, U"test"));
	ASSERT_EQUAL(tree.getChildCount(), 1);
	tree.addChild(second);
	ASSERT_EQUAL(tree.getChildCount(), 2);
	tree.addChild(handle_create<MySubClass>(34, U"foo", 56, 78));
	ASSERT_EQUAL(tree.getChildCount(), 3);
	ASSERT_EQUAL(second->getChildCount(), 0);
	second->addChild(handle_create<MyClass>(3, U"third"));
	ASSERT_EQUAL(second->getChildCount(), 1);
	ASSERT_EQUAL(tree.getChildCount(), 3);
	ASSERT_EQUAL(tree.toString(), exampleThree);

	// Tree structure from text
	Handle<Persistent> treeCopy = createPersistentClassFromText(exampleThree, U"");
	ASSERT_EQUAL(treeCopy->toString(), exampleThree);

	// Persistent string lists
	PersistentStringList myList = PersistentStringList();
	ASSERT_EQUAL(myList.value.length(), 0);
	ASSERT_EQUAL(myList.toString(), U"");

	myList = PersistentStringList(U"\"\"", U"");
	ASSERT_EQUAL(myList.value.length(), 1);
	ASSERT_EQUAL(myList.value[0], U"");
	ASSERT_EQUAL(myList.toString(), U"\"\"");

	myList = PersistentStringList(U"\"A\", \"B\"", U"");
	ASSERT_EQUAL(myList.value.length(), 2);
	ASSERT_EQUAL(myList.value[0], U"A");
	ASSERT_EQUAL(myList.value[1], U"B");
	ASSERT_EQUAL(myList.toString(), U"\"A\", \"B\"");

	myList.assignValue(U"\"Only element\"", U"");
	ASSERT_EQUAL(myList.value.length(), 1);
	ASSERT_EQUAL(myList.value[0], U"Only element");
	ASSERT_EQUAL(myList.toString(), U"\"Only element\"");

	myList = PersistentStringList(U"", U"");
	ASSERT_EQUAL(myList.value.length(), 0);
	ASSERT_EQUAL(myList.toString(), U"");

	myList.assignValue(U"\"Zero 0\", \"One 1\", \"Two 2\", \"Three 3\"", U"");
	ASSERT_EQUAL(myList.value.length(), 4);
	ASSERT_EQUAL(myList.value[0], U"Zero 0");
	ASSERT_EQUAL(myList.value[1], U"One 1");
	ASSERT_EQUAL(myList.value[2], U"Two 2");
	ASSERT_EQUAL(myList.value[3], U"Three 3");
	ASSERT_EQUAL(myList.toString(), U"\"Zero 0\", \"One 1\", \"Two 2\", \"Three 3\"");
END_TEST
