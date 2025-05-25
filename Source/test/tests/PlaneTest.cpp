
#include "../testTools.h"
#include "../../DFPSR/math/FPlane3D.h"

START_TEST(Plane)
	// Signed distance with zero offset.
	ASSERT_NEAR(FPlane3D(FVector3D( 1.4f,  0.0f ,  0.0f), 0.0f).signedDistance(FVector3D(  0.0f,  245.7f, -357.2f)),  0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(-1.2f,  0.0f ,  0.0f), 0.0f).signedDistance(FVector3D(  0.0f,  -73.6f,  864.1f)),  0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f,  0.01f,  0.0f), 0.0f).signedDistance(FVector3D(245.7f,    0.0f, -357.2f)),  0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f, -4.5f ,  0.0f), 0.0f).signedDistance(FVector3D(-73.6f,    0.0f,  864.1f)),  0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f,  0.0f ,  1.4f), 0.0f).signedDistance(FVector3D(245.7f, -357.2f,    0.0f)),  0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f,  0.0f , -2.7f), 0.0f).signedDistance(FVector3D(-73.6f,  864.1f,    0.0f)),  0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 1.4f,  0.0f ,  0.0f), 0.0f).signedDistance(FVector3D(  1.0f,  245.7f, -357.2f)),  1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(-1.2f,  0.0f ,  0.0f), 0.0f).signedDistance(FVector3D(  1.0f,  -73.6f,  864.1f)), -1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f,  0.01f,  0.0f), 0.0f).signedDistance(FVector3D(245.7f,    1.0f, -357.2f)),  1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f, -9.3f ,  0.0f), 0.0f).signedDistance(FVector3D(-73.6f,    1.0f,  864.1f)), -1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f,  0.0f ,  1.5f), 0.0f).signedDistance(FVector3D(245.7f, -357.2f,    1.0f)),  1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f,  0.0f , -2.3f), 0.0f).signedDistance(FVector3D(-73.6f,  864.1f,    1.0f)), -1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 1.4f,  0.0f ,  0.0f), 0.0f).signedDistance(FVector3D( -1.0f,  245.7f, -357.2f)), -1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(-1.2f,  0.0f ,  0.0f), 0.0f).signedDistance(FVector3D( -1.0f,  -73.6f,  864.1f)),  1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f,  0.01f,  0.0f), 0.0f).signedDistance(FVector3D(245.7f,   -1.0f, -357.2f)), -1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f, -4.2f ,  0.0f), 0.0f).signedDistance(FVector3D(-73.6f,   -1.0f,  864.1f)),  1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f,  0.0f ,  1.5f), 0.0f).signedDistance(FVector3D(245.7f, -357.2f,   -1.0f)), -1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 0.0f,  0.0f , -2.3f), 0.0f).signedDistance(FVector3D(-73.6f,  864.1f,   -1.0f)),  1.0f);
	// Signed distance with offset along X.
	ASSERT_NEAR(FPlane3D(FVector3D( 1.0f, 0.0f, 0.0f), 74.0f).signedDistance(FVector3D(75.0f, 745.1f, 135.2f)), 1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 1.0f, 0.0f, 0.0f), 74.0f).signedDistance(FVector3D(74.0f, 246.5f, 294.6f)), 0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D( 1.0f, 0.0f, 0.0f), 74.0f).signedDistance(FVector3D(73.0f, 865.7f, 625.3f)), -1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(-1.0f, 0.0f, 0.0f), 74.0f).signedDistance(FVector3D(-75.0f, 745.1f, 135.2f)), 1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(-1.0f, 0.0f, 0.0f), 74.0f).signedDistance(FVector3D(-74.0f, 246.5f, 294.6f)), 0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(-1.0f, 0.0f, 0.0f), 74.0f).signedDistance(FVector3D(-73.0f, 865.7f, 625.3f)), -1.0f);
	// Signed distance with offset along Y.
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f,  1.0f, 0.0f), 74.0f).signedDistance(FVector3D(745.1f, 75.0f, 135.2f)), 1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f,  1.0f, 0.0f), 74.0f).signedDistance(FVector3D(246.5f, 74.0f, 294.6f)), 0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f,  1.0f, 0.0f), 74.0f).signedDistance(FVector3D(865.7f, 73.0f, 625.3f)), -1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, -1.0f, 0.0f), 74.0f).signedDistance(FVector3D(745.1f, -75.0f, 135.2f)), 1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, -1.0f, 0.0f), 74.0f).signedDistance(FVector3D(246.5f, -74.0f, 294.6f)), 0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, -1.0f, 0.0f), 74.0f).signedDistance(FVector3D(865.7f, -73.0f, 625.3f)), -1.0f);
	// Signed distance with offset along Z.
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, 0.0f,  1.0f), 74.0f).signedDistance(FVector3D(745.1f, 135.2f, 75.0f)), 1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, 0.0f,  1.0f), 74.0f).signedDistance(FVector3D(246.5f, 294.6f, 74.0f)), 0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, 0.0f,  1.0f), 74.0f).signedDistance(FVector3D(865.7f, 625.3f, 73.0f)), -1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, 0.0f, -1.0f), 74.0f).signedDistance(FVector3D(745.1f, 135.2f, -75.0f)), 1.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, 0.0f, -1.0f), 74.0f).signedDistance(FVector3D(246.5f, 294.6f, -74.0f)), 0.0f);
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, 0.0f, -1.0f), 74.0f).signedDistance(FVector3D(865.7f, 625.3f, -73.0f)), -1.0f);
	// Inside or outside.
	ASSERT_EQUAL(FPlane3D(FVector3D( 1.0f, 0.0f, 0.0f), 0.0f).inside(FVector3D( 0.01f, 0.0f, 0.0f)), false);
	ASSERT_EQUAL(FPlane3D(FVector3D( 1.0f, 0.0f, 0.0f), 0.0f).inside(FVector3D(-0.01f, 0.0f, 0.0f)),  true);
	ASSERT_EQUAL(FPlane3D(FVector3D(-1.0f, 0.0f, 0.0f), 0.0f).inside(FVector3D( 0.01f, 0.0f, 0.0f)),  true);
	ASSERT_EQUAL(FPlane3D(FVector3D(-1.0f, 0.0f, 0.0f), 0.0f).inside(FVector3D(-0.01f, 0.0f, 0.0f)), false);
	ASSERT_EQUAL(FPlane3D(FVector3D( 1.0f, 0.0f, 0.0f), 45.0f).inside(FVector3D( 45.01f, 0.0f, 0.0f)), false);
	ASSERT_EQUAL(FPlane3D(FVector3D( 1.0f, 0.0f, 0.0f), 45.0f).inside(FVector3D( 44.99f, 0.0f, 0.0f)),  true);
	ASSERT_EQUAL(FPlane3D(FVector3D(-1.0f, 0.0f, 0.0f), 45.0f).inside(FVector3D(-44.99f, 0.0f, 0.0f)),  true);
	ASSERT_EQUAL(FPlane3D(FVector3D(-1.0f, 0.0f, 0.0f), 45.0f).inside(FVector3D(-45.01f, 0.0f, 0.0f)), false);
	ASSERT_EQUAL(FPlane3D(FVector3D( 1.0f, 0.0f, 0.0f), -45.0f).inside(FVector3D(-44.99f, 0.0f, 0.0f)), false);
	ASSERT_EQUAL(FPlane3D(FVector3D( 1.0f, 0.0f, 0.0f), -45.0f).inside(FVector3D(-45.01f, 0.0f, 0.0f)),  true);
	ASSERT_EQUAL(FPlane3D(FVector3D(-1.0f, 0.0f, 0.0f), -45.0f).inside(FVector3D( 45.01f, 0.0f, 0.0f)),  true);
	ASSERT_EQUAL(FPlane3D(FVector3D(-1.0f, 0.0f, 0.0f), -45.0f).inside(FVector3D( 44.99f, 0.0f, 0.0f)), false);
	// Ray intersection.
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, 0.0f,  1.8f), 123.45f).rayIntersect(FVector3D(12.3f, 45.6f, -26.0f), FVector3D(0.0f, 0.0f,  1.2f)), FVector3D(12.3f, 45.6f,  123.45f));
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, 0.0f,  1.2f), 123.45f).rayIntersect(FVector3D(22.5f, 74.8f,  42.0f), FVector3D(0.0f, 0.0f, -1.4f)), FVector3D(22.5f, 74.8f,  123.45f));
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, 0.0f, -0.3f), 123.45f).rayIntersect(FVector3D(82.6f, 27.4f,  83.0f), FVector3D(0.0f, 0.0f,  0.1f)), FVector3D(82.6f, 27.4f, -123.45f));
	ASSERT_NEAR(FPlane3D(FVector3D(0.0f, 0.0f, -9.6f), 123.45f).rayIntersect(FVector3D(-6.3f, 53.0f, -45.0f), FVector3D(0.0f, 0.0f, -1.6f)), FVector3D(-6.3f, 53.0f, -123.45f));
END_TEST
