
#include "../testTools.h"

START_TEST(Vector)
	// Comparisons
	ASSERT_EQUAL(UVector2D(64u, 23u), UVector2D(64u, 23u));
	ASSERT_EQUAL(UVector3D(64u, 23u, 74u), UVector3D(64u, 23u, 74u));
	ASSERT_EQUAL(UVector4D(64u, 23u, 74u, 1483u), UVector4D(64u, 23u, 74u, 1483u));
	ASSERT_EQUAL(IVector2D(-34, 764), IVector2D(-34, 764));
	ASSERT_NOT_EQUAL(IVector2D(67, 34), IVector2D(57, 34));
	ASSERT_NOT_EQUAL(IVector2D(84, -26), IVector2D(84, 35));
	ASSERT_EQUAL(IVector3D(75, 25, -7), IVector3D(75, 25, -7));
	ASSERT_EQUAL(IVector4D(84, -2, 34, 245), IVector4D(84, -2, 34, 245));
	// Construct from single element
	ASSERT_EQUAL(IVector2D(58), IVector2D(58, 58));
	ASSERT_EQUAL(IVector3D(45), IVector3D(45, 45, 45));
	ASSERT_EQUAL(IVector4D(98), IVector4D(98, 98, 98, 98));
	// Access single element
	ASSERT_EQUAL(IVector2D(12, 34)[0], 12);
	ASSERT_EQUAL(IVector2D(12, 34)[1], 34);
	ASSERT_EQUAL(IVector3D(12, 34, 56)[0], 12);
	ASSERT_EQUAL(IVector3D(12, 34, 56)[1], 34);
	ASSERT_EQUAL(IVector3D(12, 34, 56)[2], 56);
	ASSERT_EQUAL(IVector4D(12, 34, 56, 78)[0], 12);
	ASSERT_EQUAL(IVector4D(12, 34, 56, 78)[1], 34);
	ASSERT_EQUAL(IVector4D(12, 34, 56, 78)[2], 56);
	ASSERT_EQUAL(IVector4D(12, 34, 56, 78)[3], 78);
	// Math
	ASSERT_EQUAL(IVector2D(12, 34) + IVector2D(8, 6), IVector2D(20, 40));
	ASSERT_EQUAL(IVector2D(12, 34) + 4, IVector2D(16, 38));
	ASSERT_EQUAL(100 + IVector2D(1, 2), IVector2D(101, 102));
	ASSERT_EQUAL(IVector2D(13, 64) - IVector2D(3, 4), IVector2D(10, 60));
	ASSERT_EQUAL(IVector2D(37, 38) - 30, IVector2D(7, 8));
	ASSERT_EQUAL(10 - IVector2D(4, 7), IVector2D(6, 3));
	ASSERT_EQUAL(IVector2D(3, -8) * IVector2D(2, -2), IVector2D(6, 16));
	ASSERT_EQUAL(IVector2D(2, 3) * 5, IVector2D(10, 15));
	ASSERT_EQUAL(7 * IVector2D(1, -1), IVector2D(7, -7));
	ASSERT_EQUAL(IVector2D(20, 40) / IVector2D(2, 4), IVector2D(10, 10));
	ASSERT_EQUAL(IVector2D(20, 40) / 10, IVector2D(2, 4));
	ASSERT_EQUAL(12 / IVector2D(3, 4), IVector2D(4, 3));
END_TEST

