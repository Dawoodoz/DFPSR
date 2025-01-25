
#include "../testTools.h"
#include "../../DFPSR/base/Handle.h"

static int64_t countA = 0;
static int64_t countB = 0;
static int64_t countC = 0;

struct TypeA {
	int32_t value;
	TypeA(int32_t value) : value(value) {
		countA++;
	}
	~TypeA() {
		countA--;
	}
};

struct TypeB {
	Handle<TypeA> left;
	Handle<TypeA> right;
	TypeB() { countB++; }
	TypeB(const Handle<TypeA> &left) : left(left) { countB++; }
	TypeB(const Handle<TypeA> &left, const Handle<TypeA> &right) : left(left), right(right) { countB++; }
	~TypeB() { countB--; }
};

struct TypeC {
	Handle<TypeA> x;
	Handle<TypeB> y;
	TypeC() { countC++; }
	TypeC(const Handle<TypeA> &x) : x(x) { countC++; }
	TypeC(const Handle<TypeA> &x, const Handle<TypeB> &y) : x(x), y(y) { countC++; }
	~TypeC() { countC--; }
};

START_TEST(Handle)
	{
		ASSERT_EQUAL(countA, 0);
		ASSERT_EQUAL(countB, 0);
		ASSERT_EQUAL(countC, 0);
		{
			TypeA valueA = 35;
			ASSERT_EQUAL(countA, 1);
			ASSERT_EQUAL(countB, 0);
			ASSERT_EQUAL(countC, 0);
			ASSERT_EQUAL(valueA.value, 35);
			{
				TypeB pairB = TypeB(handle_create<TypeA>(5), handle_create<TypeA>(8));
				ASSERT_EQUAL(countA, 3);
				ASSERT_EQUAL(countB, 1);
				ASSERT_EQUAL(countC, 0);
				ASSERT_EQUAL(pairB.left->value, 5);
				ASSERT_EQUAL(pairB.right->value, 8);
				{
					TypeC trio = TypeC(handle_create<TypeA>(1), handle_create<TypeB>(handle_create<TypeA>(2), handle_create<TypeA>(3)));
					ASSERT_EQUAL(countA, 6);
					ASSERT_EQUAL(countB, 2);
					ASSERT_EQUAL(countC, 1);
					ASSERT_EQUAL(trio.x->value, 1);
					ASSERT_EQUAL(trio.y->left->value, 2);
					ASSERT_EQUAL(trio.y->right->value, 3);
				}
				ASSERT_EQUAL(countA, 3);
				ASSERT_EQUAL(countB, 1);
				ASSERT_EQUAL(countC, 0);
				ASSERT_EQUAL(pairB.left->value, 5);
				ASSERT_EQUAL(pairB.right->value, 8);
			}
			ASSERT_EQUAL(countA, 1);
			ASSERT_EQUAL(countB, 0);
			ASSERT_EQUAL(countC, 0);
			ASSERT_EQUAL(valueA.value, 35);
		}
		ASSERT_EQUAL(countA, 0);
		ASSERT_EQUAL(countB, 0);
		ASSERT_EQUAL(countC, 0);
	}
END_TEST
