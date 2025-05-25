
#include "../testTools.h"
#include "../../DFPSR/math/UVector.h"
#include "../../DFPSR/math/IVector.h"
#include "../../DFPSR/math/LVector.h"
#include "../../DFPSR/math/FVector.h"

START_TEST(Vector)
	// Comparisons
	ASSERT_EQUAL(UVector2D(64u, 23u), UVector2D(64u, 23u));
	ASSERT_EQUAL(IVector2D(64, 23), IVector2D(64, 23));
	ASSERT_EQUAL(LVector2D(64, 23), LVector2D(64, 23));
	ASSERT_NEAR(FVector2D(64.0f, 23.0f), FVector2D(64.0f, 23.0f));
	ASSERT_EQUAL(UVector3D(64u, 23u, 74u), UVector3D(64u, 23u, 74u));
	ASSERT_EQUAL(IVector3D(64, 23, 74), IVector3D(64, 23, 74));
	ASSERT_EQUAL(LVector3D(64, 23, 74), LVector3D(64, 23, 74));
	ASSERT_NEAR(FVector3D(64.0f, 23.0f, 74.0f), FVector3D(64.0f, 23.0f, 74.0f));
	ASSERT_EQUAL(UVector4D(64u, 23u, 74u, 1483u), UVector4D(64u, 23u, 74u, 1483u));
	ASSERT_EQUAL(IVector4D(64, 23, 74, 1483), IVector4D(64, 23, 74, 1483));
	ASSERT_EQUAL(LVector4D(64, 23, 74, 1483), LVector4D(64, 23, 74, 1483));
	ASSERT_EQUAL(FVector4D(64.0f, 23.0f, 74.0f, 1483.0f), FVector4D(64.0f, 23.0f, 74.0f, 1483.0f));
	// Construct from single element
	ASSERT_EQUAL(UVector2D(58u), UVector2D(58u, 58u));
	ASSERT_EQUAL(UVector3D(45u), UVector3D(45u, 45u, 45u));
	ASSERT_EQUAL(UVector4D(98u), UVector4D(98u, 98u, 98u, 98u));
	ASSERT_EQUAL(IVector2D(58), IVector2D(58, 58));
	ASSERT_EQUAL(IVector3D(45), IVector3D(45, 45, 45));
	ASSERT_EQUAL(IVector4D(98), IVector4D(98, 98, 98, 98));
	ASSERT_EQUAL(LVector2D(58), LVector2D(58, 58));
	ASSERT_EQUAL(LVector3D(45), LVector3D(45, 45, 45));
	ASSERT_EQUAL(LVector4D(98), LVector4D(98, 98, 98, 98));
	ASSERT_EQUAL(FVector2D(58.1f), FVector2D(58.1f, 58.1f));
	ASSERT_EQUAL(FVector3D(45.2f), FVector3D(45.2f, 45.2f, 45.2f));
	ASSERT_EQUAL(FVector4D(98.3f), FVector4D(98.3f, 98.3f, 98.3f, 98.3f));
	// Access single element
	ASSERT_EQUAL(UVector2D(12u, 34u)[0], 12u);
	ASSERT_EQUAL(UVector2D(12u, 34u)[1], 34u);
	ASSERT_EQUAL(UVector3D(12u, 34u, 56u)[0], 12u);
	ASSERT_EQUAL(UVector3D(12u, 34u, 56u)[1], 34u);
	ASSERT_EQUAL(UVector3D(12u, 34u, 56u)[2], 56u);
	ASSERT_EQUAL(UVector4D(12u, 34u, 56u, 78u)[0], 12u);
	ASSERT_EQUAL(UVector4D(12u, 34u, 56u, 78u)[1], 34u);
	ASSERT_EQUAL(UVector4D(12u, 34u, 56u, 78u)[2], 56u);
	ASSERT_EQUAL(UVector4D(12u, 34u, 56u, 78u)[3], 78u);
	ASSERT_EQUAL(IVector2D(12, 34)[0], 12);
	ASSERT_EQUAL(IVector2D(12, 34)[1], 34);
	ASSERT_EQUAL(IVector3D(12, 34, 56)[0], 12);
	ASSERT_EQUAL(IVector3D(12, 34, 56)[1], 34);
	ASSERT_EQUAL(IVector3D(12, 34, 56)[2], 56);
	ASSERT_EQUAL(IVector4D(12, 34, 56, 78)[0], 12);
	ASSERT_EQUAL(IVector4D(12, 34, 56, 78)[1], 34);
	ASSERT_EQUAL(IVector4D(12, 34, 56, 78)[2], 56);
	ASSERT_EQUAL(IVector4D(12, 34, 56, 78)[3], 78);
	ASSERT_EQUAL(LVector2D(12, 34)[0], 12);
	ASSERT_EQUAL(LVector2D(12, 34)[1], 34);
	ASSERT_EQUAL(LVector3D(12, 34, 56)[0], 12);
	ASSERT_EQUAL(LVector3D(12, 34, 56)[1], 34);
	ASSERT_EQUAL(LVector3D(12, 34, 56)[2], 56);
	ASSERT_EQUAL(LVector4D(12, 34, 56, 78)[0], 12);
	ASSERT_EQUAL(LVector4D(12, 34, 56, 78)[1], 34);
	ASSERT_EQUAL(LVector4D(12, 34, 56, 78)[2], 56);
	ASSERT_EQUAL(LVector4D(12, 34, 56, 78)[3], 78);
	ASSERT_EQUAL(FVector2D(12.0f, 34.0f)[0], 12.0f);
	ASSERT_EQUAL(FVector2D(12.0f, 34.0f)[1], 34.0f);
	ASSERT_EQUAL(FVector3D(12.0f, 34.0f, 56.0f)[0], 12.0f);
	ASSERT_EQUAL(FVector3D(12.0f, 34.0f, 56.0f)[1], 34.0f);
	ASSERT_EQUAL(FVector3D(12.0f, 34.0f, 56.0f)[2], 56.0f);
	ASSERT_EQUAL(FVector4D(12.0f, 34.0f, 56.0f, 78.0f)[0], 12.0f);
	ASSERT_EQUAL(FVector4D(12.0f, 34.0f, 56.0f, 78.0f)[1], 34.0f);
	ASSERT_EQUAL(FVector4D(12.0f, 34.0f, 56.0f, 78.0f)[2], 56.0f);
	ASSERT_EQUAL(FVector4D(12.0f, 34.0f, 56.0f, 78.0f)[3], 78.0f);
	// Math
	ASSERT_EQUAL(UVector2D(12u, 34u) + UVector2D(8u, 6u), UVector2D(20u, 40u));
	ASSERT_EQUAL(IVector2D(12, 34) + IVector2D(8, 6), IVector2D(20, 40));
	ASSERT_EQUAL(LVector2D(12, 34) + LVector2D(8, 6), LVector2D(20, 40));
	ASSERT_NEAR(FVector2D(12.0f, 34.0f) + FVector2D(8.0f, 6.0f), FVector2D(20.0f, 40.0f));
	ASSERT_EQUAL(UVector2D(12u, 34u) + 4u, UVector2D(16u, 38u));
	ASSERT_EQUAL(IVector2D(12, 34) + 4, IVector2D(16, 38));
	ASSERT_EQUAL(LVector2D(12, 34) + 4, LVector2D(16, 38));
	ASSERT_NEAR(FVector2D(12.0f, 34.0f) + 4.0f, FVector2D(16.0f, 38.0f));
	ASSERT_EQUAL(100u + UVector2D(1u, 2u), UVector2D(101u, 102u));
	ASSERT_EQUAL(100 + IVector2D(1, 2), IVector2D(101, 102));
	ASSERT_EQUAL(100 + LVector2D(1, 2), LVector2D(101, 102));
	ASSERT_NEAR(100.0f + FVector2D(1.0f, 2.0f), FVector2D(101.0f, 102.0f));
	ASSERT_EQUAL(UVector2D(13u, 64u) - UVector2D(3u, 4u), UVector2D(10u, 60u));
	ASSERT_EQUAL(IVector2D(13, 64) - IVector2D(3, 4), IVector2D(10, 60));
	ASSERT_EQUAL(LVector2D(13, 64) - LVector2D(3, 4), LVector2D(10, 60));
	ASSERT_NEAR(FVector2D(13.0f, 64.0f) - FVector2D(3.0f, 4.0f), FVector2D(10.0f, 60.0f));
	ASSERT_EQUAL(UVector2D(37u, 38u) - 30u, UVector2D(7u, 8u));
	ASSERT_EQUAL(IVector2D(37, 38) - 30, IVector2D(7, 8));
	ASSERT_EQUAL(LVector2D(37, 38) - 30, LVector2D(7, 8));
	ASSERT_NEAR(FVector2D(37.0f, 38.0f) - 30.0f, FVector2D(7.0f, 8.0f));
	ASSERT_EQUAL(10u - UVector2D(4u, 7u), UVector2D(6u, 3u));
	ASSERT_EQUAL(10 - IVector2D(4, 7), IVector2D(6, 3));
	ASSERT_EQUAL(10 - LVector2D(4, 7), LVector2D(6, 3));
	ASSERT_NEAR(10.0f - FVector2D(4.0f, 7.0f), FVector2D(6.0f, 3.0f));
	ASSERT_EQUAL(IVector2D(3, -8) * IVector2D(2, -2), IVector2D(6, 16));
	ASSERT_EQUAL(LVector2D(3, -8) * LVector2D(2, -2), LVector2D(6, 16));
	ASSERT_NEAR(FVector2D(3.0f, -8.0f) * FVector2D(2.0f, -2.0f), FVector2D(6.0f, 16.0f));
	ASSERT_EQUAL(UVector2D(2u, 3u) * 5u, UVector2D(10u, 15u));
	ASSERT_EQUAL(IVector2D(2, 3) * 5, IVector2D(10, 15));
	ASSERT_EQUAL(LVector2D(2, 3) * 5, LVector2D(10, 15));
	ASSERT_NEAR(FVector2D(2.0f, 3.0f) * 5.0f, FVector2D(10.0f, 15.0f));
	ASSERT_EQUAL(7 * IVector2D(1, -1), IVector2D(7, -7));
	ASSERT_EQUAL(7 * LVector2D(1, -1), LVector2D(7, -7));
	ASSERT_NEAR(7.0f * FVector2D(1.0f, -1.0f), FVector2D(7.0f, -7.0f));
	ASSERT_EQUAL(UVector2D(20u, 40u) / UVector2D(2u, 4u), UVector2D(10u, 10u));
	ASSERT_EQUAL(IVector2D(20, 40) / IVector2D(2, 4), IVector2D(10, 10));
	ASSERT_EQUAL(LVector2D(20, 40) / LVector2D(2, 4), LVector2D(10, 10));
	ASSERT_NEAR(FVector2D(20.0f, 40.0f) / FVector2D(2.0f, 4.0f), FVector2D(10.0f, 10.0f));
	ASSERT_EQUAL(UVector2D(20u, 40u) / 10u, UVector2D(2u, 4u));
	ASSERT_EQUAL(IVector2D(20, 40) / 10, IVector2D(2, 4));
	ASSERT_EQUAL(LVector2D(20, 40) / 10, LVector2D(2, 4));
	ASSERT_NEAR(FVector2D(20.0f, 40.0f) / 10.0f, FVector2D(2.0f, 4.0f));
	ASSERT_EQUAL(12u / UVector2D(3u, 4u), UVector2D(4u, 3u));
	ASSERT_EQUAL(12 / IVector2D(3, 4), IVector2D(4, 3));
	ASSERT_EQUAL(12 / LVector2D(3, 4), LVector2D(4, 3));
	ASSERT_NEAR(12.0f / FVector2D(3.0f, 4.0f), FVector2D(4.0f, 3.0f));
END_TEST

