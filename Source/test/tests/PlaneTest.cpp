
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
	// Inside or outside
	ASSERT_EQUAL(FPlane3D(FVector3D( 1.0f,  0.0f ,  0.0f), 0.0f).inside(FVector3D( 0.01f, 0.0f, 0.0f)), false);
	ASSERT_EQUAL(FPlane3D(FVector3D( 1.0f,  0.0f ,  0.0f), 0.0f).inside(FVector3D(-0.01f, 0.0f, 0.0f)),  true);
	ASSERT_EQUAL(FPlane3D(FVector3D(-1.0f,  0.0f ,  0.0f), 0.0f).inside(FVector3D( 0.01f, 0.0f, 0.0f)),  true);
	ASSERT_EQUAL(FPlane3D(FVector3D(-1.0f,  0.0f ,  0.0f), 0.0f).inside(FVector3D(-0.01f, 0.0f, 0.0f)), false);
END_TEST
