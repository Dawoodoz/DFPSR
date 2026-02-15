
#include "../testTools.h"
#include "../../DFPSR/base/simd.h"
#include "../../DFPSR/base/endian.h"

// TODO: Set up a test where SIMD is disabled to force using the reference implementation.
// TODO: Keep the reference implementation alongside the SIMD types during brute-force testing with millions of random inputs.

#define ASSERT_EQUAL_SIMD(A, B) ASSERT_COMP(A, B, allLanesEqual, U"==")
#define ASSERT_NOTEQUAL_SIMD(A, B) ASSERT_COMP(A, B, !allLanesEqual, U"!=")

static void testComparisons() {
	// Test non-vectorized comparison functions. (Used for test conditions and debug assertions)
	ASSERT_EQUAL(allLanesEqual(I32x4(-2, 1, 4, 7345), I32x4(-2, 1, 4, 7345)), true);
	ASSERT_EQUAL(allLanesEqual(I32x4(-2, 1, 4, 7345), I32x4( 2, 1, 4, 7345)), false);
	ASSERT_EQUAL(allLanesEqual(I32x4(-2, 1, 4, 7345), I32x4(-2, 5, 4, 7345)), false);
	ASSERT_EQUAL(allLanesEqual(I32x4(-2, 1, 4, 7345), I32x4(-2, 1, 2, 7345)), false);
	ASSERT_EQUAL(allLanesEqual(I32x4(-2, 1, 4, 7345), I32x4(-2, 1, 4, 6531)), false);
	ASSERT_EQUAL(allLanesEqual(I32x4(-2, 1, 4, 7345), I32x4(-2, 0, 4,  385)), false);
	ASSERT_EQUAL(allLanesEqual(I32x4( 0, 0, 0,    0), I32x4(-2, 1, 4, 7345)), false);
	ASSERT_EQUAL(allLanesNotEqual(I32x4(-2, 1, 4, 5), I32x4( 6, 8, 3, 7)), true);
	ASSERT_EQUAL(allLanesNotEqual(I32x4(-2, 1, 4, 5), I32x4(-2, 8, 3, 7)), false);
	ASSERT_EQUAL(allLanesNotEqual(I32x4(-2, 1, 4, 5), I32x4( 6, 1, 3, 7)), false);
	ASSERT_EQUAL(allLanesNotEqual(I32x4(-2, 1, 4, 5), I32x4( 6, 8, 4, 7)), false);
	ASSERT_EQUAL(allLanesNotEqual(I32x4(-2, 1, 4, 5), I32x4( 6, 8, 3, 5)), false);
	ASSERT_EQUAL(allLanesNotEqual(I32x4(-2, 1, 4, 5), I32x4(-2, 8, 3, 5)), false);
	ASSERT_EQUAL(allLanesNotEqual(I32x4(-2, 1, 4, 5), I32x4( 6, 1, 4, 7)), false);
	ASSERT_EQUAL(allLanesLesser (I32x4(-4, -1,  1,  3), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesLesser (I32x4(-3, -1,  1,  3), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (I32x4(-4,  0,  1,  3), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (I32x4(-4, -1,  2,  3), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (I32x4(-4, -1,  1,  4), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (I32x4(36, -1,  1,  3), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (I32x4(-4, 86,  1,  3), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (I32x4(-4, -1, 35,  3), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (I32x4(-4, -1,  1, 75), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(I32x4(-2,  1,  4,  5), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesGreater(I32x4(-3,  1,  4,  5), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(I32x4(-2,  0,  4,  5), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(I32x4(-2,  1,  2,  5), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(I32x4(-2,  1,  4,  4), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(I32x4(-5,  1,  4,  5), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(I32x4(-2, -5,  4,  5), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(I32x4(-2,  1, -7,  5), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(I32x4(-2,  1,  4, -4), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x4(-4, -1,  1,  3), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x4(-3, -1,  1,  3), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x4(-4,  0,  1,  3), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x4(-4, -1,  2,  3), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x4(-4, -1,  1,  4), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x4(36, -1,  1,  3), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x4(-4, 86,  1,  3), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x4(-4, -1, 35,  3), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x4(-4, -1,  1, 75), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x4(-2,  1,  4,  5), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x4(-3,  1,  4,  5), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x4(-2,  0,  4,  5), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x4(-2,  1,  2,  5), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x4(-2,  1,  4,  4), I32x4(-3,  0,  2,  4)), true);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x4(-5,  1,  4,  5), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x4(-2, -5,  4,  5), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x4(-2,  1, -7,  5), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x4(-2,  1,  4, -4), I32x4(-3,  0,  2,  4)), false);
	ASSERT_EQUAL(allLanesEqual         (I32x8(-2, 1, 4, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesEqual         (I32x8( 0, 1, 4, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (I32x8(-2, 0, 4, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (I32x8(-2, 1, 0, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (I32x8(-2, 1, 4, 0, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (I32x8(-2, 1, 4, 8,  0, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (I32x8(-2, 1, 4, 8, 74,  0, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (I32x8(-2, 1, 4, 8, 74, 23, 0, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (I32x8(-2, 1, 4, 8, 74, 23, 5,  0), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (I32x8( 5, 8, 6, 9, 35, 75, 3, 75), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesNotEqual      (I32x8(-2, 8, 6, 9, 35, 75, 3, 75), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (I32x8( 5, 1, 6, 9, 35, 75, 3, 75), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (I32x8( 5, 8, 4, 9, 35, 75, 3, 75), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (I32x8( 5, 8, 6, 8, 35, 75, 3, 75), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (I32x8( 5, 8, 6, 9, 74, 75, 3, 75), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (I32x8( 5, 8, 6, 9, 35, 23, 3, 75), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (I32x8( 5, 8, 6, 9, 35, 75, 5, 75), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (I32x8( 5, 8, 6, 9, 35, 75, 3, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (I32x8(-2, 1, 4, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesser        (I32x8(-3, 0, 3, 7, 73, 22, 4, 63), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesGreater       (I32x8(-1, 2, 5, 9, 75, 24, 6, 65), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesGreater       (I32x8(-2, 2, 5, 9, 75, 24, 6, 65), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (I32x8(-1, 0, 5, 9, 75, 24, 6, 65), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (I32x8(-1, 2, 4, 9, 75, 24, 6, 65), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (I32x8(-1, 2, 5, 8, 75, 24, 6, 65), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (I32x8(-1, 2, 5, 9,  3, 24, 6, 65), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (I32x8(-1, 2, 5, 9, 75, 23, 6, 65), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (I32x8(-1, 2, 5, 9, 75, 24, 2, 65), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (I32x8(-1, 2, 5, 9, 75, 24, 6,  5), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x8(-2, 1, 4, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x8(-1, 1, 4, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x8(-2, 2, 4, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x8(-2, 1, 5, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x8(-2, 1, 4, 9, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x8(-2, 1, 4, 8, 75, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x8(-2, 1, 4, 8, 74, 73, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x8(-2, 1, 4, 8, 74, 23, 6, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (I32x8(-2, 1, 4, 8, 74, 23, 5, 69), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x8(-2, 1, 4, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x8(-3, 1, 4, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x8(-2, 0, 4, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x8(-2, 1, 2, 8, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x8(-2, 1, 4, 5, 74, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x8(-2, 1, 4, 8, 34, 23, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x8(-2, 1, 4, 8, 74,  1, 5, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x8(-2, 1, 4, 8, 74, 23, 3, 64), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(I32x8(-2, 1, 4, 8, 74, 23, 5,  4), I32x8(-2, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual(U32x4(8, 1, 4, 7345), U32x4(8, 1, 4, 7345)), true);
	ASSERT_EQUAL(allLanesEqual(U32x4(8, 1, 4, 7345), U32x4(2, 1, 4, 7345)), false);
	ASSERT_EQUAL(allLanesEqual(U32x4(8, 1, 4, 7345), U32x4(8, 5, 4, 7345)), false);
	ASSERT_EQUAL(allLanesEqual(U32x4(8, 1, 4, 7345), U32x4(8, 1, 2, 7345)), false);
	ASSERT_EQUAL(allLanesEqual(U32x4(8, 1, 4, 7345), U32x4(8, 1, 4, 6531)), false);
	ASSERT_EQUAL(allLanesNotEqual(U32x4(8, 1, 4, 5), U32x4(6, 8, 3, 7)), true);
	ASSERT_EQUAL(allLanesNotEqual(U32x4(8, 1, 4, 5), U32x4(8, 8, 3, 7)), false);
	ASSERT_EQUAL(allLanesNotEqual(U32x4(8, 1, 4, 5), U32x4(6, 1, 3, 7)), false);
	ASSERT_EQUAL(allLanesNotEqual(U32x4(8, 1, 4, 5), U32x4(6, 8, 4, 7)), false);
	ASSERT_EQUAL(allLanesNotEqual(U32x4(8, 1, 4, 5), U32x4(6, 8, 3, 5)), false);
	ASSERT_EQUAL(allLanesLesser (U32x4( 7, 4,  1,  3), U32x4( 8,  5,  2,  4)), true);
	ASSERT_EQUAL(allLanesLesser (U32x4( 8, 4,  1,  3), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (U32x4( 7, 5,  1,  3), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (U32x4( 7, 4,  2,  3), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (U32x4( 7, 4,  1,  4), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (U32x4(36, 4,  1,  3), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (U32x4( 7,48,  1,  3), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (U32x4( 7, 4, 35,  3), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesser (U32x4( 7, 4,  1, 75), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(U32x4( 9, 6,  3,  5), U32x4( 8,  5,  2,  4)), true);
	ASSERT_EQUAL(allLanesGreater(U32x4( 8, 6,  3,  5), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(U32x4( 9, 5,  3,  5), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(U32x4( 9, 6,  2,  5), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(U32x4( 9, 6,  3,  4), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(U32x4( 4, 6,  3,  5), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(U32x4( 9, 2,  3,  5), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(U32x4( 9, 6,  1,  5), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesGreater(U32x4( 9, 6,  3,  0), U32x4( 8,  5,  2,  4)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x4( 6, 9, 1, 3), U32x4(7,10, 2, 4)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x4( 7, 9, 1, 3), U32x4(7,10, 2, 4)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x4( 6,10, 1, 3), U32x4(7,10, 2, 4)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x4( 6, 9, 2, 3), U32x4(7,10, 2, 4)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x4( 6, 9, 1, 4), U32x4(7,10, 2, 4)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x4(36, 9, 1, 3), U32x4(7,10, 2, 4)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x4( 6,86, 1, 3), U32x4(7,10, 2, 4)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x4( 6, 9,35, 3), U32x4(7,10, 2, 4)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x4( 6, 9, 1,75), U32x4(7,10, 2, 4)), false);
	ASSERT_EQUAL(allLanesEqual         (U32x8( 8, 1, 4, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesEqual         (U32x8( 0, 1, 4, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (U32x8( 8, 0, 4, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (U32x8( 8, 1, 0, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (U32x8( 8, 1, 4, 0, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (U32x8( 8, 1, 4, 8,  0, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (U32x8( 8, 1, 4, 8, 74,  0, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (U32x8( 8, 1, 4, 8, 74, 23, 0, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesEqual         (U32x8( 8, 1, 4, 8, 74, 23, 5,  0), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (U32x8( 5, 8, 6, 9, 35, 75, 3, 75), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesNotEqual      (U32x8( 8, 8, 6, 9, 35, 75, 3, 75), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (U32x8( 5, 1, 6, 9, 35, 75, 3, 75), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (U32x8( 5, 8, 4, 9, 35, 75, 3, 75), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (U32x8( 5, 8, 6, 8, 35, 75, 3, 75), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (U32x8( 5, 8, 6, 9, 74, 75, 3, 75), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (U32x8( 5, 8, 6, 9, 35, 23, 3, 75), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (U32x8( 5, 8, 6, 9, 35, 75, 5, 75), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (U32x8( 5, 8, 6, 9, 35, 75, 3, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesNotEqual      (U32x8( 8, 1, 4, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesser        (U32x8( 7, 0, 3, 7, 73, 22, 4, 63), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesGreater       (U32x8( 9, 2, 5, 9, 75, 24, 6, 65), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesGreater       (U32x8( 8, 2, 5, 9, 75, 24, 6, 65), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (U32x8( 9, 0, 5, 9, 75, 24, 6, 65), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (U32x8( 9, 2, 4, 9, 75, 24, 6, 65), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (U32x8( 9, 2, 5, 8, 75, 24, 6, 65), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (U32x8( 9, 2, 5, 9,  3, 24, 6, 65), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (U32x8( 9, 2, 5, 9, 75, 23, 6, 65), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (U32x8( 9, 2, 5, 9, 75, 24, 2, 65), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreater       (U32x8( 9, 2, 5, 9, 75, 24, 6,  5), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x8( 8, 1, 4, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x8( 9, 1, 4, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x8( 8, 2, 4, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x8( 8, 1, 5, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x8( 8, 1, 4, 9, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x8( 8, 1, 4, 8, 75, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x8( 8, 1, 4, 8, 74, 73, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x8( 8, 1, 4, 8, 74, 23, 6, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesLesserOrEqual (U32x8( 8, 1, 4, 8, 74, 23, 5, 69), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(U32x8( 8, 1, 4, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), true);
	ASSERT_EQUAL(allLanesGreaterOrEqual(U32x8( 7, 1, 4, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(U32x8( 8, 0, 4, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(U32x8( 8, 1, 2, 8, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(U32x8( 8, 1, 4, 5, 74, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(U32x8( 8, 1, 4, 8, 34, 23, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(U32x8( 8, 1, 4, 8, 74,  1, 5, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(U32x8( 8, 1, 4, 8, 74, 23, 3, 64), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);
	ASSERT_EQUAL(allLanesGreaterOrEqual(U32x8( 8, 1, 4, 8, 74, 23, 5,  4), U32x8( 8, 1, 4, 8, 74, 23, 5, 64)), false);

	// F32x4 Comparisons
	ASSERT_EQUAL_SIMD(F32x4(1.5f), F32x4(1.5f, 1.5f, 1.5f, 1.5f));
	ASSERT_EQUAL_SIMD(F32x4(-1.5f), F32x4(-1.5f, -1.5f, -1.5f, -1.5f));
	ASSERT_EQUAL_SIMD(F32x4(1.2f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, 7.8f));
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().x, 1.2f);
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().y, 3.4f);
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().z, 5.6f);
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().w, 7.8f);
	ASSERT_NOTEQUAL_SIMD(F32x4(1.3f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, 7.8f));
	ASSERT_NOTEQUAL_SIMD(F32x4(1.2f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, -1.4f, 5.6f, 7.8f));
	ASSERT_NOTEQUAL_SIMD(F32x4(1.2f, 3.4f, 5.5f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, 7.8f));
	ASSERT_NOTEQUAL_SIMD(F32x4(1.2f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, -7.8f));

	// F32x8 Comparisons
	ASSERT_EQUAL_SIMD(F32x8(1.5f), F32x8(1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f));
	ASSERT_EQUAL_SIMD(F32x8(-1.5f), F32x8(-1.5f, -1.5f, -1.5f, -1.5f, -1.5f, -1.5f, -1.5f, -1.5f));
	ASSERT_EQUAL_SIMD(F32x8(1.2f, 3.4f, 5.6f, 7.8f, -2.4f, 452.351f, 1000000.0f, -1000.0f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, -2.4f, 452.351f, 1000000.0f, -1000.0f));
	ASSERT_NOTEQUAL_SIMD(F32x8(1.3f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f));
	ASSERT_NOTEQUAL_SIMD(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, -1.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f));
	ASSERT_NOTEQUAL_SIMD(F32x8(1.2f, 3.4f, 5.5f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f));
	ASSERT_NOTEQUAL_SIMD(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, -7.8f, 5.3f, 6.7f, 1.4f, -5.2f));
	ASSERT_NOTEQUAL_SIMD(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 0.0f, 6.7f, 1.4f, -5.2f));
	ASSERT_NOTEQUAL_SIMD(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.69f, 1.4f, -5.2f));
	ASSERT_NOTEQUAL_SIMD(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.3f, -5.2f));
	ASSERT_NOTEQUAL_SIMD(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, 5.2f));

	// I32x4 Comparisons
	ASSERT_EQUAL_SIMD(I32x4(4), I32x4(4, 4, 4, 4));
	ASSERT_EQUAL_SIMD(I32x4(-4), I32x4(-4, -4, -4, -4));
	ASSERT_EQUAL_SIMD(I32x4(-1, 2, -3, 4), I32x4(-1, 2, -3, 4));
	ASSERT_NOTEQUAL_SIMD(I32x4(-1, 2, 7, 4), I32x4(-1, 2, -3, 4));

	// I32x8 Comparisons
	ASSERT_EQUAL_SIMD(I32x8(4), I32x8(4, 4, 4, 4, 4, 4, 4, 4));
	ASSERT_EQUAL_SIMD(I32x8(-4), I32x8(-4, -4, -4, -4, -4, -4, -4, -4));
	ASSERT_EQUAL_SIMD(I32x8(-1, 2, -3, 4, -5, 6, -7, 8), I32x8(-1, 2, -3, 4, -5, 6, -7, 8));
	ASSERT_NOTEQUAL_SIMD(I32x8(-1, 2, 7, 4, 8, 3, 5, 45), I32x8(-1, 2, -3, 4, 8, 3, 5, 45));

	// U32x4 Comparisons
	ASSERT_EQUAL_SIMD(U32x4(4), U32x4(4, 4, 4, 4));
	ASSERT_EQUAL_SIMD(U32x4(1, 2, 3, 4), U32x4(1, 2, 3, 4));
	ASSERT_NOTEQUAL_SIMD(U32x4(1, 2, 7, 4), U32x4(1, 2, 3, 4));

	// U32x8 Comparisons
	ASSERT_EQUAL_SIMD(U32x8(4), U32x8(4, 4, 4, 4, 4, 4, 4, 4));
	ASSERT_EQUAL_SIMD(U32x8(1, 2, 3, 4, 5, 6, 7, 8), U32x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U32x8(1, 2, 3, 4, 5, 6, 12, 8), U32x8(1, 2, 3, 4, 5, 6, 7, 8));

	// U16x8 Comparisons
	ASSERT_EQUAL_SIMD(U16x8(8), U16x8(8, 8, 8, 8, 8, 8, 8, 8));
	ASSERT_EQUAL_SIMD(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(0, 2, 3, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(1, 0, 3, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(1, 2, 0, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(1, 2, 3, 0, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(1, 2, 3, 4, 0, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(1, 2, 3, 4, 5, 0, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(1, 2, 3, 4, 5, 6, 0, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(1, 2, 3, 4, 5, 6, 7, 0), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(1, 2, 0, 4, 5, 0, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(1, 0, 3, 4, 5, 6, 0, 0), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(0, 2, 3, 4, 0, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOTEQUAL_SIMD(U16x8(0, 0, 0, 0, 0, 0, 0, 0), U16x8(1, 2, 3, 4, 5, 6, 7, 8));

	// U16x16 Comparisons
	ASSERT_EQUAL_SIMD(U16x16(8), U16x16(8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8));
	ASSERT_EQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 0, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 0, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 0, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 0, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9,  0, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,  0, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,  0, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,  0, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,  0, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,  0), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 2, 0, 4, 5, 0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(1, 0, 3, 4, 5, 6, 0, 0, 9, 10, 11, 12, 13,  0, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(0, 2, 3, 4, 0, 6, 7, 8, 9, 10, 11, 0,  13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_NOTEQUAL_SIMD(U16x16(0, 0, 0, 0, 0, 0, 0, 0, 9, 10, 11, 0,  13, 14,  0, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));

	// U8x16 Comparisons
	ASSERT_EQUAL_SIMD(U8x16(250), U8x16(250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250));
	ASSERT_EQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 0, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 0, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 0, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 0, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 0, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 0, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 0, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 0, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 0, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 0, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 0, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 0), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 251, 252, 6, 254, 255), U8x16(1, 2, 3, 4, 5, 9, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOTEQUAL_SIMD(U8x16(1, 2, 3, 0, 5, 6, 7, 8, 9, 0, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 4, 8, 9, 10, 250, 251, 252, 253, 254, 255));

	// U8x32 Comparisons
	ASSERT_EQUAL_SIMD(U8x32((uint8_t)250), U8x32(250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250));
	ASSERT_NOTEQUAL_SIMD(U8x32((uint8_t)250), U8x32(250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 100, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250));
	ASSERT_NOTEQUAL_SIMD(U8x32((uint8_t)250), U8x32(0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250));
	ASSERT_NOTEQUAL_SIMD(U8x32((uint8_t)250), U8x32(250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0));
}

static void testBitMasks() {
	ASSERT_EQUAL_SIMD(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) & 0x0000FFFF, U32x4(0x0000FFFF, 0x00005678, 0x0000F0F0, 0x00000000));
	ASSERT_EQUAL_SIMD(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) & 0xFFFF0000, U32x4(0xFFFF0000, 0x12340000, 0xF0F00000, 0x00000000));
	ASSERT_EQUAL_SIMD(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) | 0x0000FFFF, U32x4(0xFFFFFFFF, 0x1234FFFF, 0xF0F0FFFF, 0x0000FFFF));
	ASSERT_EQUAL_SIMD(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) | 0xFFFF0000, U32x4(0xFFFFFFFF, 0xFFFF5678, 0xFFFFF0F0, 0xFFFF0000));
	ASSERT_EQUAL_SIMD(U32x4(0xFFFFFFFF, 0xFFF000FF, 0xF0F0F0F0, 0x12345678) & U32x4(0xFF00FF00, 0xFFFF0000, 0x000FF000, 0x0FF00FF0), U32x4(0xFF00FF00, 0xFFF00000, 0x0000F000, 0x02300670));
	ASSERT_EQUAL_SIMD(U32x4(0xF00F000F, 0xFFF000FF, 0x10010011, 0xABC00000) | U32x4(0x0000FF00, 0xFFFF0000, 0x000FF000, 0x000DEF00), U32x4(0xF00FFF0F, 0xFFFF00FF, 0x100FF011, 0xABCDEF00));
	ASSERT_EQUAL_SIMD(U32x4(0xFFFFFFFF, 0x01234567, 0xF0F0F0F0, 0x00000000) ^ 0x0000FFFF, U32x4(0xFFFF0000, 0x0123BA98, 0xF0F00F0F, 0x0000FFFF));
	ASSERT_EQUAL_SIMD(
	    U32x8(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  & 0x0000FFFF,
	    U32x8(0x0000FFFF, 0x00005678, 0x0000F0F0, 0x00000000, 0x0000EEEE, 0x00004321, 0x00000F0F, 0x00000001));
	ASSERT_EQUAL_SIMD(
	    U32x8(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  & 0xFFFF0000,
	    U32x8(0xFFFF0000, 0x12340000, 0xF0F00000, 0x00000000, 0xEEEE0000, 0x87650000, 0x0F0F0000, 0x00010000));
	ASSERT_EQUAL_SIMD(
	    U32x8(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  | 0x0000FFFF,
	    U32x8(0xFFFFFFFF, 0x1234FFFF, 0xF0F0FFFF, 0x0000FFFF, 0xEEEEFFFF, 0x8765FFFF, 0x0F0FFFFF, 0x0001FFFF));
	ASSERT_EQUAL_SIMD(
	    U32x8(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  | 0xFFFF0000,
	    U32x8(0xFFFFFFFF, 0xFFFF5678, 0xFFFFF0F0, 0xFFFF0000, 0xFFFFEEEE, 0xFFFF4321, 0xFFFF0F0F, 0xFFFF0001));
	ASSERT_EQUAL_SIMD(
	    U32x8(0xFFFFFFFF, 0xFFF000FF, 0xF0F0F0F0, 0x12345678, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  & U32x8(0xFF00FF00, 0xFFFF0000, 0x000FF000, 0x0FF00FF0, 0xF00FF00F, 0x00FFFF00, 0xF0F0F0F0, 0x0000FFFF),
	    U32x8(0xFF00FF00, 0xFFF00000, 0x0000F000, 0x02300670, 0xE00EE00E, 0x00654300, 0x00000000, 0x00000001));
	ASSERT_EQUAL_SIMD(
	    U32x8(0xFFFFFFFF, 0xFFF000FF, 0xF0F0F0F0, 0x12345678, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  | U32x8(0xFF00FF00, 0xFFFF0000, 0x000FF000, 0x0FF00FF0, 0xF00FF00F, 0x00FFFF00, 0xF0F0F0F0, 0x0000FFFF),
	    U32x8(0xFFFFFFFF, 0xFFFF00FF, 0xF0FFF0F0, 0x1FF45FF8, 0xFEEFFEEF, 0x87FFFF21, 0xFFFFFFFF, 0x0001FFFF));
	ASSERT_EQUAL_SIMD(
	    U32x8(0b11001100110000110101010010110011, 0b00101011001011101010001101111001, 0b11001010000110111010010100101100, 0b01010111010001010010101110010110, 0b10101110100110100010101011011001, 0b00101110100111010001101010110000, 0b11101010001011100010101110001111, 0b00101010111100010110010110001000)
	  ^ U32x8(0b00101101001110100011010010100001, 0b10101110100101000011101001010011, 0b00101011100101001011000010100100, 0b11010011101001000110010110110111, 0b00111100101000101010001101001010, 0b00101110100110000111110011010101, 0b11001010010101010010110010101000, 0b11110000111100001111000011110000),
	    U32x8(0b11100001111110010110000000010010, 0b10000101101110101001100100101010, 0b11100001100011110001010110001000, 0b10000100111000010100111000100001, 0b10010010001110001000100110010011, 0b00000000000001010110011001100101, 0b00100000011110110000011100100111, 0b11011010000000011001010101111000));
}

static void testBitShift() {
	// Bit shift with dynamic uniform offset.
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 0,
	                  U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 1,
	                  U16x8(0b1000110110010110, 0b1010101101001100, 0b1001000101100110, 0b1101001011001010, 0b1011001100101010, 0b0110011000011100, 0b0100101010010110, 0b0101101100100100));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 2,
	                  U16x8(0b0001101100101100, 0b0101011010011000, 0b0010001011001100, 0b1010010110010100, 0b0110011001010100, 0b1100110000111000, 0b1001010100101100, 0b1011011001001000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 3,
	                  U16x8(0b0011011001011000, 0b1010110100110000, 0b0100010110011000, 0b0100101100101000, 0b1100110010101000, 0b1001100001110000, 0b0010101001011000, 0b0110110010010000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 4,
	                  U16x8(0b0110110010110000, 0b0101101001100000, 0b1000101100110000, 0b1001011001010000, 0b1001100101010000, 0b0011000011100000, 0b0101010010110000, 0b1101100100100000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 5,
	                  U16x8(0b1101100101100000, 0b1011010011000000, 0b0001011001100000, 0b0010110010100000, 0b0011001010100000, 0b0110000111000000, 0b1010100101100000, 0b1011001001000000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 6,
	                  U16x8(0b1011001011000000, 0b0110100110000000, 0b0010110011000000, 0b0101100101000000, 0b0110010101000000, 0b1100001110000000, 0b0101001011000000, 0b0110010010000000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 7,
	                  U16x8(0b0110010110000000, 0b1101001100000000, 0b0101100110000000, 0b1011001010000000, 0b1100101010000000, 0b1000011100000000, 0b1010010110000000, 0b1100100100000000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 8,
	                  U16x8(0b1100101100000000, 0b1010011000000000, 0b1011001100000000, 0b0110010100000000, 0b1001010100000000, 0b0000111000000000, 0b0100101100000000, 0b1001001000000000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 9,
	                  U16x8(0b1001011000000000, 0b0100110000000000, 0b0110011000000000, 0b1100101000000000, 0b0010101000000000, 0b0001110000000000, 0b1001011000000000, 0b0010010000000000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 10,
	                  U16x8(0b0010110000000000, 0b1001100000000000, 0b1100110000000000, 0b1001010000000000, 0b0101010000000000, 0b0011100000000000, 0b0010110000000000, 0b0100100000000000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 11,
	                  U16x8(0b0101100000000000, 0b0011000000000000, 0b1001100000000000, 0b0010100000000000, 0b1010100000000000, 0b0111000000000000, 0b0101100000000000, 0b1001000000000000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 12,
	                  U16x8(0b1011000000000000, 0b0110000000000000, 0b0011000000000000, 0b0101000000000000, 0b0101000000000000, 0b1110000000000000, 0b1011000000000000, 0b0010000000000000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 13,
	                  U16x8(0b0110000000000000, 0b1100000000000000, 0b0110000000000000, 0b1010000000000000, 0b1010000000000000, 0b1100000000000000, 0b0110000000000000, 0b0100000000000000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 14,
	                  U16x8(0b1100000000000000, 0b1000000000000000, 0b1100000000000000, 0b0100000000000000, 0b0100000000000000, 0b1000000000000000, 0b1100000000000000, 0b1000000000000000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 15,
	                  U16x8(0b1000000000000000, 0b0000000000000000, 0b1000000000000000, 0b1000000000000000, 0b1000000000000000, 0b0000000000000000, 0b1000000000000000, 0b0000000000000000));
	ASSERT_CRASH(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) << 16, U"Tried to shift ");
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 0,
	                  U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 1,
	                  U16x8(0b0110001101100101, 0b0010101011010011, 0b0110010001011001, 0b0011010010110010, 0b0010110011001010, 0b0001100110000111, 0b0101001010100101, 0b0001011011001001));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 2,
	                  U16x8(0b0011000110110010, 0b0001010101101001, 0b0011001000101100, 0b0001101001011001, 0b0001011001100101, 0b0000110011000011, 0b0010100101010010, 0b0000101101100100));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 3,
	                  U16x8(0b0001100011011001, 0b0000101010110100, 0b0001100100010110, 0b0000110100101100, 0b0000101100110010, 0b0000011001100001, 0b0001010010101001, 0b0000010110110010));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 4,
	                  U16x8(0b0000110001101100, 0b0000010101011010, 0b0000110010001011, 0b0000011010010110, 0b0000010110011001, 0b0000001100110000, 0b0000101001010100, 0b0000001011011001));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 5,
	                  U16x8(0b0000011000110110, 0b0000001010101101, 0b0000011001000101, 0b0000001101001011, 0b0000001011001100, 0b0000000110011000, 0b0000010100101010, 0b0000000101101100));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 6,
	                  U16x8(0b0000001100011011, 0b0000000101010110, 0b0000001100100010, 0b0000000110100101, 0b0000000101100110, 0b0000000011001100, 0b0000001010010101, 0b0000000010110110));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 7,
	                  U16x8(0b0000000110001101, 0b0000000010101011, 0b0000000110010001, 0b0000000011010010, 0b0000000010110011, 0b0000000001100110, 0b0000000101001010, 0b0000000001011011));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 8,
	                  U16x8(0b0000000011000110, 0b0000000001010101, 0b0000000011001000, 0b0000000001101001, 0b0000000001011001, 0b0000000000110011, 0b0000000010100101, 0b0000000000101101));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 9,
	                  U16x8(0b0000000001100011, 0b0000000000101010, 0b0000000001100100, 0b0000000000110100, 0b0000000000101100, 0b0000000000011001, 0b0000000001010010, 0b0000000000010110));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 10,
	                  U16x8(0b0000000000110001, 0b0000000000010101, 0b0000000000110010, 0b0000000000011010, 0b0000000000010110, 0b0000000000001100, 0b0000000000101001, 0b0000000000001011));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 11,
	                  U16x8(0b0000000000011000, 0b0000000000001010, 0b0000000000011001, 0b0000000000001101, 0b0000000000001011, 0b0000000000000110, 0b0000000000010100, 0b0000000000000101));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 12,
	                  U16x8(0b0000000000001100, 0b0000000000000101, 0b0000000000001100, 0b0000000000000110, 0b0000000000000101, 0b0000000000000011, 0b0000000000001010, 0b0000000000000010));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 13,
	                  U16x8(0b0000000000000110, 0b0000000000000010, 0b0000000000000110, 0b0000000000000011, 0b0000000000000010, 0b0000000000000001, 0b0000000000000101, 0b0000000000000001));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 14,
	                  U16x8(0b0000000000000011, 0b0000000000000001, 0b0000000000000011, 0b0000000000000001, 0b0000000000000001, 0b0000000000000000, 0b0000000000000010, 0b0000000000000000));
	ASSERT_EQUAL_SIMD(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 15,
	                  U16x8(0b0000000000000001, 0b0000000000000000, 0b0000000000000001, 0b0000000000000000, 0b0000000000000000, 0b0000000000000000, 0b0000000000000001, 0b0000000000000000));
	ASSERT_CRASH(U16x8(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010) >> 16, U"Tried to shift ");
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 0,
	                  U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 1,
	                  U32x4(0b10001101100101101010101101001100, 0b10010001011001101101001011001010, 0b10110011001010100110011000011100, 0b01001010100101100101101100100100));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 2,
	                  U32x4(0b00011011001011010101011010011000, 0b00100010110011011010010110010100, 0b01100110010101001100110000111000, 0b10010101001011001011011001001000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 3,
	                  U32x4(0b00110110010110101010110100110000, 0b01000101100110110100101100101000, 0b11001100101010011001100001110000, 0b00101010010110010110110010010000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 4,
	                  U32x4(0b01101100101101010101101001100000, 0b10001011001101101001011001010000, 0b10011001010100110011000011100000, 0b01010100101100101101100100100000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 5,
	                  U32x4(0b11011001011010101011010011000000, 0b00010110011011010010110010100000, 0b00110010101001100110000111000000, 0b10101001011001011011001001000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 6,
	                  U32x4(0b10110010110101010110100110000000, 0b00101100110110100101100101000000, 0b01100101010011001100001110000000, 0b01010010110010110110010010000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 7,
	                  U32x4(0b01100101101010101101001100000000, 0b01011001101101001011001010000000, 0b11001010100110011000011100000000, 0b10100101100101101100100100000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 8,
	                  U32x4(0b11001011010101011010011000000000, 0b10110011011010010110010100000000, 0b10010101001100110000111000000000, 0b01001011001011011001001000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 9,
	                  U32x4(0b10010110101010110100110000000000, 0b01100110110100101100101000000000, 0b00101010011001100001110000000000, 0b10010110010110110010010000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 10,
	                  U32x4(0b00101101010101101001100000000000, 0b11001101101001011001010000000000, 0b01010100110011000011100000000000, 0b00101100101101100100100000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 11,
	                  U32x4(0b01011010101011010011000000000000, 0b10011011010010110010100000000000, 0b10101001100110000111000000000000, 0b01011001011011001001000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 12,
	                  U32x4(0b10110101010110100110000000000000, 0b00110110100101100101000000000000, 0b01010011001100001110000000000000, 0b10110010110110010010000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 13,
	                  U32x4(0b01101010101101001100000000000000, 0b01101101001011001010000000000000, 0b10100110011000011100000000000000, 0b01100101101100100100000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 14,
	                  U32x4(0b11010101011010011000000000000000, 0b11011010010110010100000000000000, 0b01001100110000111000000000000000, 0b11001011011001001000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 15,
	                  U32x4(0b10101010110100110000000000000000, 0b10110100101100101000000000000000, 0b10011001100001110000000000000000, 0b10010110110010010000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 16,
	                  U32x4(0b01010101101001100000000000000000, 0b01101001011001010000000000000000, 0b00110011000011100000000000000000, 0b00101101100100100000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 17,
	                  U32x4(0b10101011010011000000000000000000, 0b11010010110010100000000000000000, 0b01100110000111000000000000000000, 0b01011011001001000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 18,
	                  U32x4(0b01010110100110000000000000000000, 0b10100101100101000000000000000000, 0b11001100001110000000000000000000, 0b10110110010010000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 19,
	                  U32x4(0b10101101001100000000000000000000, 0b01001011001010000000000000000000, 0b10011000011100000000000000000000, 0b01101100100100000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 20,
	                  U32x4(0b01011010011000000000000000000000, 0b10010110010100000000000000000000, 0b00110000111000000000000000000000, 0b11011001001000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 21,
	                  U32x4(0b10110100110000000000000000000000, 0b00101100101000000000000000000000, 0b01100001110000000000000000000000, 0b10110010010000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 22,
	                  U32x4(0b01101001100000000000000000000000, 0b01011001010000000000000000000000, 0b11000011100000000000000000000000, 0b01100100100000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 23,
	                  U32x4(0b11010011000000000000000000000000, 0b10110010100000000000000000000000, 0b10000111000000000000000000000000, 0b11001001000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 24,
	                  U32x4(0b10100110000000000000000000000000, 0b01100101000000000000000000000000, 0b00001110000000000000000000000000, 0b10010010000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 25,
	                  U32x4(0b01001100000000000000000000000000, 0b11001010000000000000000000000000, 0b00011100000000000000000000000000, 0b00100100000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 26,
	                  U32x4(0b10011000000000000000000000000000, 0b10010100000000000000000000000000, 0b00111000000000000000000000000000, 0b01001000000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 27,
	                  U32x4(0b00110000000000000000000000000000, 0b00101000000000000000000000000000, 0b01110000000000000000000000000000, 0b10010000000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 28,
	                  U32x4(0b01100000000000000000000000000000, 0b01010000000000000000000000000000, 0b11100000000000000000000000000000, 0b00100000000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 29,
	                  U32x4(0b11000000000000000000000000000000, 0b10100000000000000000000000000000, 0b11000000000000000000000000000000, 0b01000000000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 30,
	                  U32x4(0b10000000000000000000000000000000, 0b01000000000000000000000000000000, 0b10000000000000000000000000000000, 0b10000000000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 31,
	                  U32x4(0b00000000000000000000000000000000, 0b10000000000000000000000000000000, 0b00000000000000000000000000000000, 0b00000000000000000000000000000000));
	ASSERT_CRASH(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) << 32, U"Tried to shift ");
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 0,
	                  U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 1,
	                  U32x4(0b01100011011001011010101011010011, 0b01100100010110011011010010110010, 0b00101100110010101001100110000111, 0b01010010101001011001011011001001));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 2,
	                  U32x4(0b00110001101100101101010101101001, 0b00110010001011001101101001011001, 0b00010110011001010100110011000011, 0b00101001010100101100101101100100));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 3,
	                  U32x4(0b00011000110110010110101010110100, 0b00011001000101100110110100101100, 0b00001011001100101010011001100001, 0b00010100101010010110010110110010));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 4,
	                  U32x4(0b00001100011011001011010101011010, 0b00001100100010110011011010010110, 0b00000101100110010101001100110000, 0b00001010010101001011001011011001));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 5,
	                  U32x4(0b00000110001101100101101010101101, 0b00000110010001011001101101001011, 0b00000010110011001010100110011000, 0b00000101001010100101100101101100));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 6,
	                  U32x4(0b00000011000110110010110101010110, 0b00000011001000101100110110100101, 0b00000001011001100101010011001100, 0b00000010100101010010110010110110));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 7,
	                  U32x4(0b00000001100011011001011010101011, 0b00000001100100010110011011010010, 0b00000000101100110010101001100110, 0b00000001010010101001011001011011));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 8,
	                  U32x4(0b00000000110001101100101101010101, 0b00000000110010001011001101101001, 0b00000000010110011001010100110011, 0b00000000101001010100101100101101));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 9,
	                  U32x4(0b00000000011000110110010110101010, 0b00000000011001000101100110110100, 0b00000000001011001100101010011001, 0b00000000010100101010010110010110));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 10,
	                  U32x4(0b00000000001100011011001011010101, 0b00000000001100100010110011011010, 0b00000000000101100110010101001100, 0b00000000001010010101001011001011));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 11,
	                  U32x4(0b00000000000110001101100101101010, 0b00000000000110010001011001101101, 0b00000000000010110011001010100110, 0b00000000000101001010100101100101));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 12,
	                  U32x4(0b00000000000011000110110010110101, 0b00000000000011001000101100110110, 0b00000000000001011001100101010011, 0b00000000000010100101010010110010));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 13,
	                  U32x4(0b00000000000001100011011001011010, 0b00000000000001100100010110011011, 0b00000000000000101100110010101001, 0b00000000000001010010101001011001));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 14,
	                  U32x4(0b00000000000000110001101100101101, 0b00000000000000110010001011001101, 0b00000000000000010110011001010100, 0b00000000000000101001010100101100));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 15,
	                  U32x4(0b00000000000000011000110110010110, 0b00000000000000011001000101100110, 0b00000000000000001011001100101010, 0b00000000000000010100101010010110));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 16,
	                  U32x4(0b00000000000000001100011011001011, 0b00000000000000001100100010110011, 0b00000000000000000101100110010101, 0b00000000000000001010010101001011));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 17,
	                  U32x4(0b00000000000000000110001101100101, 0b00000000000000000110010001011001, 0b00000000000000000010110011001010, 0b00000000000000000101001010100101));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 18,
	                  U32x4(0b00000000000000000011000110110010, 0b00000000000000000011001000101100, 0b00000000000000000001011001100101, 0b00000000000000000010100101010010));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 19,
	                  U32x4(0b00000000000000000001100011011001, 0b00000000000000000001100100010110, 0b00000000000000000000101100110010, 0b00000000000000000001010010101001));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 20,
	                  U32x4(0b00000000000000000000110001101100, 0b00000000000000000000110010001011, 0b00000000000000000000010110011001, 0b00000000000000000000101001010100));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 21,
	                  U32x4(0b00000000000000000000011000110110, 0b00000000000000000000011001000101, 0b00000000000000000000001011001100, 0b00000000000000000000010100101010));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 22,
	                  U32x4(0b00000000000000000000001100011011, 0b00000000000000000000001100100010, 0b00000000000000000000000101100110, 0b00000000000000000000001010010101));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 23,
	                  U32x4(0b00000000000000000000000110001101, 0b00000000000000000000000110010001, 0b00000000000000000000000010110011, 0b00000000000000000000000101001010));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 24,
	                  U32x4(0b00000000000000000000000011000110, 0b00000000000000000000000011001000, 0b00000000000000000000000001011001, 0b00000000000000000000000010100101));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 25,
	                  U32x4(0b00000000000000000000000001100011, 0b00000000000000000000000001100100, 0b00000000000000000000000000101100, 0b00000000000000000000000001010010));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 26,
	                  U32x4(0b00000000000000000000000000110001, 0b00000000000000000000000000110010, 0b00000000000000000000000000010110, 0b00000000000000000000000000101001));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 27,
	                  U32x4(0b00000000000000000000000000011000, 0b00000000000000000000000000011001, 0b00000000000000000000000000001011, 0b00000000000000000000000000010100));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 28,
	                  U32x4(0b00000000000000000000000000001100, 0b00000000000000000000000000001100, 0b00000000000000000000000000000101, 0b00000000000000000000000000001010));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 29,
	                  U32x4(0b00000000000000000000000000000110, 0b00000000000000000000000000000110, 0b00000000000000000000000000000010, 0b00000000000000000000000000000101));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 30,
	                  U32x4(0b00000000000000000000000000000011, 0b00000000000000000000000000000011, 0b00000000000000000000000000000001, 0b00000000000000000000000000000010));
	ASSERT_EQUAL_SIMD(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 31,
	                  U32x4(0b00000000000000000000000000000001, 0b00000000000000000000000000000001, 0b00000000000000000000000000000000, 0b00000000000000000000000000000001));
	ASSERT_CRASH(U32x4(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010) >> 32, U"Tried to shift ");
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 0,
	                  U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 1,
	                  U16x16(0b1000110110010110, 0b1010101101001100, 0b1001000101100110, 0b1101001011001010, 0b1011001100101010, 0b0110011000011100, 0b0100101010010110, 0b0101101100100100, 0b1110010110100100, 0b0001011010100110, 0b1011001000111010, 0b0101011101001010, 0b0111010100101000, 0b1101001010011000, 0b1010001110001010, 0b0110101010010100));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 2,
	                  U16x16(0b0001101100101100, 0b0101011010011000, 0b0010001011001100, 0b1010010110010100, 0b0110011001010100, 0b1100110000111000, 0b1001010100101100, 0b1011011001001000, 0b1100101101001000, 0b0010110101001100, 0b0110010001110100, 0b1010111010010100, 0b1110101001010000, 0b1010010100110000, 0b0100011100010100, 0b1101010100101000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 3,
	                  U16x16(0b0011011001011000, 0b1010110100110000, 0b0100010110011000, 0b0100101100101000, 0b1100110010101000, 0b1001100001110000, 0b0010101001011000, 0b0110110010010000, 0b1001011010010000, 0b0101101010011000, 0b1100100011101000, 0b0101110100101000, 0b1101010010100000, 0b0100101001100000, 0b1000111000101000, 0b1010101001010000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 4,
	                  U16x16(0b0110110010110000, 0b0101101001100000, 0b1000101100110000, 0b1001011001010000, 0b1001100101010000, 0b0011000011100000, 0b0101010010110000, 0b1101100100100000, 0b0010110100100000, 0b1011010100110000, 0b1001000111010000, 0b1011101001010000, 0b1010100101000000, 0b1001010011000000, 0b0001110001010000, 0b0101010010100000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 5,
	                  U16x16(0b1101100101100000, 0b1011010011000000, 0b0001011001100000, 0b0010110010100000, 0b0011001010100000, 0b0110000111000000, 0b1010100101100000, 0b1011001001000000, 0b0101101001000000, 0b0110101001100000, 0b0010001110100000, 0b0111010010100000, 0b0101001010000000, 0b0010100110000000, 0b0011100010100000, 0b1010100101000000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 6,
	                  U16x16(0b1011001011000000, 0b0110100110000000, 0b0010110011000000, 0b0101100101000000, 0b0110010101000000, 0b1100001110000000, 0b0101001011000000, 0b0110010010000000, 0b1011010010000000, 0b1101010011000000, 0b0100011101000000, 0b1110100101000000, 0b1010010100000000, 0b0101001100000000, 0b0111000101000000, 0b0101001010000000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 7,
	                  U16x16(0b0110010110000000, 0b1101001100000000, 0b0101100110000000, 0b1011001010000000, 0b1100101010000000, 0b1000011100000000, 0b1010010110000000, 0b1100100100000000, 0b0110100100000000, 0b1010100110000000, 0b1000111010000000, 0b1101001010000000, 0b0100101000000000, 0b1010011000000000, 0b1110001010000000, 0b1010010100000000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 8,
	                  U16x16(0b1100101100000000, 0b1010011000000000, 0b1011001100000000, 0b0110010100000000, 0b1001010100000000, 0b0000111000000000, 0b0100101100000000, 0b1001001000000000, 0b1101001000000000, 0b0101001100000000, 0b0001110100000000, 0b1010010100000000, 0b1001010000000000, 0b0100110000000000, 0b1100010100000000, 0b0100101000000000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 9,
	                  U16x16(0b1001011000000000, 0b0100110000000000, 0b0110011000000000, 0b1100101000000000, 0b0010101000000000, 0b0001110000000000, 0b1001011000000000, 0b0010010000000000, 0b1010010000000000, 0b1010011000000000, 0b0011101000000000, 0b0100101000000000, 0b0010100000000000, 0b1001100000000000, 0b1000101000000000, 0b1001010000000000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 10,
	                  U16x16(0b0010110000000000, 0b1001100000000000, 0b1100110000000000, 0b1001010000000000, 0b0101010000000000, 0b0011100000000000, 0b0010110000000000, 0b0100100000000000, 0b0100100000000000, 0b0100110000000000, 0b0111010000000000, 0b1001010000000000, 0b0101000000000000, 0b0011000000000000, 0b0001010000000000, 0b0010100000000000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 11,
	                  U16x16(0b0101100000000000, 0b0011000000000000, 0b1001100000000000, 0b0010100000000000, 0b1010100000000000, 0b0111000000000000, 0b0101100000000000, 0b1001000000000000, 0b1001000000000000, 0b1001100000000000, 0b1110100000000000, 0b0010100000000000, 0b1010000000000000, 0b0110000000000000, 0b0010100000000000, 0b0101000000000000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 12,
	                  U16x16(0b1011000000000000, 0b0110000000000000, 0b0011000000000000, 0b0101000000000000, 0b0101000000000000, 0b1110000000000000, 0b1011000000000000, 0b0010000000000000, 0b0010000000000000, 0b0011000000000000, 0b1101000000000000, 0b0101000000000000, 0b0100000000000000, 0b1100000000000000, 0b0101000000000000, 0b1010000000000000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 13,
	                  U16x16(0b0110000000000000, 0b1100000000000000, 0b0110000000000000, 0b1010000000000000, 0b1010000000000000, 0b1100000000000000, 0b0110000000000000, 0b0100000000000000, 0b0100000000000000, 0b0110000000000000, 0b1010000000000000, 0b1010000000000000, 0b1000000000000000, 0b1000000000000000, 0b1010000000000000, 0b0100000000000000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 14,
	                  U16x16(0b1100000000000000, 0b1000000000000000, 0b1100000000000000, 0b0100000000000000, 0b0100000000000000, 0b1000000000000000, 0b1100000000000000, 0b1000000000000000, 0b1000000000000000, 0b1100000000000000, 0b0100000000000000, 0b0100000000000000, 0b0000000000000000, 0b0000000000000000, 0b0100000000000000, 0b1000000000000000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 15,
	                  U16x16(0b1000000000000000, 0b0000000000000000, 0b1000000000000000, 0b1000000000000000, 0b1000000000000000, 0b0000000000000000, 0b1000000000000000, 0b0000000000000000, 0b0000000000000000, 0b1000000000000000, 0b1000000000000000, 0b1000000000000000, 0b0000000000000000, 0b0000000000000000, 0b1000000000000000, 0b0000000000000000));
	ASSERT_CRASH(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) << 16, U"Tried to shift ");
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 0,
	                  U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 1,
	                  U16x16(0b0110001101100101, 0b0010101011010011, 0b0110010001011001, 0b0011010010110010, 0b0010110011001010, 0b0001100110000111, 0b0101001010100101, 0b0001011011001001, 0b0011100101101001, 0b0100010110101001, 0b0010110010001110, 0b0001010111010010, 0b0001110101001010, 0b0011010010100110, 0b0110100011100010, 0b0001101010100101));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 2,
	                  U16x16(0b0011000110110010, 0b0001010101101001, 0b0011001000101100, 0b0001101001011001, 0b0001011001100101, 0b0000110011000011, 0b0010100101010010, 0b0000101101100100, 0b0001110010110100, 0b0010001011010100, 0b0001011001000111, 0b0000101011101001, 0b0000111010100101, 0b0001101001010011, 0b0011010001110001, 0b0000110101010010));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 3,
	                  U16x16(0b0001100011011001, 0b0000101010110100, 0b0001100100010110, 0b0000110100101100, 0b0000101100110010, 0b0000011001100001, 0b0001010010101001, 0b0000010110110010, 0b0000111001011010, 0b0001000101101010, 0b0000101100100011, 0b0000010101110100, 0b0000011101010010, 0b0000110100101001, 0b0001101000111000, 0b0000011010101001));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 4,
	                  U16x16(0b0000110001101100, 0b0000010101011010, 0b0000110010001011, 0b0000011010010110, 0b0000010110011001, 0b0000001100110000, 0b0000101001010100, 0b0000001011011001, 0b0000011100101101, 0b0000100010110101, 0b0000010110010001, 0b0000001010111010, 0b0000001110101001, 0b0000011010010100, 0b0000110100011100, 0b0000001101010100));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 5,
	                  U16x16(0b0000011000110110, 0b0000001010101101, 0b0000011001000101, 0b0000001101001011, 0b0000001011001100, 0b0000000110011000, 0b0000010100101010, 0b0000000101101100, 0b0000001110010110, 0b0000010001011010, 0b0000001011001000, 0b0000000101011101, 0b0000000111010100, 0b0000001101001010, 0b0000011010001110, 0b0000000110101010));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 6,
	                  U16x16(0b0000001100011011, 0b0000000101010110, 0b0000001100100010, 0b0000000110100101, 0b0000000101100110, 0b0000000011001100, 0b0000001010010101, 0b0000000010110110, 0b0000000111001011, 0b0000001000101101, 0b0000000101100100, 0b0000000010101110, 0b0000000011101010, 0b0000000110100101, 0b0000001101000111, 0b0000000011010101));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 7,
	                  U16x16(0b0000000110001101, 0b0000000010101011, 0b0000000110010001, 0b0000000011010010, 0b0000000010110011, 0b0000000001100110, 0b0000000101001010, 0b0000000001011011, 0b0000000011100101, 0b0000000100010110, 0b0000000010110010, 0b0000000001010111, 0b0000000001110101, 0b0000000011010010, 0b0000000110100011, 0b0000000001101010));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 8,
	                  U16x16(0b0000000011000110, 0b0000000001010101, 0b0000000011001000, 0b0000000001101001, 0b0000000001011001, 0b0000000000110011, 0b0000000010100101, 0b0000000000101101, 0b0000000001110010, 0b0000000010001011, 0b0000000001011001, 0b0000000000101011, 0b0000000000111010, 0b0000000001101001, 0b0000000011010001, 0b0000000000110101));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 9,
	                  U16x16(0b0000000001100011, 0b0000000000101010, 0b0000000001100100, 0b0000000000110100, 0b0000000000101100, 0b0000000000011001, 0b0000000001010010, 0b0000000000010110, 0b0000000000111001, 0b0000000001000101, 0b0000000000101100, 0b0000000000010101, 0b0000000000011101, 0b0000000000110100, 0b0000000001101000, 0b0000000000011010));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 10,
	                  U16x16(0b0000000000110001, 0b0000000000010101, 0b0000000000110010, 0b0000000000011010, 0b0000000000010110, 0b0000000000001100, 0b0000000000101001, 0b0000000000001011, 0b0000000000011100, 0b0000000000100010, 0b0000000000010110, 0b0000000000001010, 0b0000000000001110, 0b0000000000011010, 0b0000000000110100, 0b0000000000001101));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 11,
	                  U16x16(0b0000000000011000, 0b0000000000001010, 0b0000000000011001, 0b0000000000001101, 0b0000000000001011, 0b0000000000000110, 0b0000000000010100, 0b0000000000000101, 0b0000000000001110, 0b0000000000010001, 0b0000000000001011, 0b0000000000000101, 0b0000000000000111, 0b0000000000001101, 0b0000000000011010, 0b0000000000000110));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 12,
	                  U16x16(0b0000000000001100, 0b0000000000000101, 0b0000000000001100, 0b0000000000000110, 0b0000000000000101, 0b0000000000000011, 0b0000000000001010, 0b0000000000000010, 0b0000000000000111, 0b0000000000001000, 0b0000000000000101, 0b0000000000000010, 0b0000000000000011, 0b0000000000000110, 0b0000000000001101, 0b0000000000000011));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 13,
	                  U16x16(0b0000000000000110, 0b0000000000000010, 0b0000000000000110, 0b0000000000000011, 0b0000000000000010, 0b0000000000000001, 0b0000000000000101, 0b0000000000000001, 0b0000000000000011, 0b0000000000000100, 0b0000000000000010, 0b0000000000000001, 0b0000000000000001, 0b0000000000000011, 0b0000000000000110, 0b0000000000000001));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 14,
	                  U16x16(0b0000000000000011, 0b0000000000000001, 0b0000000000000011, 0b0000000000000001, 0b0000000000000001, 0b0000000000000000, 0b0000000000000010, 0b0000000000000000, 0b0000000000000001, 0b0000000000000010, 0b0000000000000001, 0b0000000000000000, 0b0000000000000000, 0b0000000000000001, 0b0000000000000011, 0b0000000000000000));
	ASSERT_EQUAL_SIMD(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 15,
	                  U16x16(0b0000000000000001, 0b0000000000000000, 0b0000000000000001, 0b0000000000000000, 0b0000000000000000, 0b0000000000000000, 0b0000000000000001, 0b0000000000000000, 0b0000000000000000, 0b0000000000000001, 0b0000000000000000, 0b0000000000000000, 0b0000000000000000, 0b0000000000000000, 0b0000000000000001, 0b0000000000000000));
	ASSERT_CRASH(U16x16(0b1100011011001011, 0b0101010110100110, 0b1100100010110011, 0b0110100101100101, 0b0101100110010101, 0b0011001100001110, 0b1010010101001011, 0b0010110110010010, 0b0111001011010010, 0b1000101101010011, 0b0101100100011101, 0b0010101110100101, 0b0011101010010100, 0b0110100101001100, 0b1101000111000101, 0b0011010101001010) >> 16, U"Tried to shift ");
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 0,
	                  U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 1,
	                  U32x8(0b10001101100101101010101101001100, 0b10010001011001101101001011001010, 0b10110011001010100110011000011100, 0b01001010100101100101101100100100, 0b10110100011011010101001011010110, 0b10110101011011001011010110101010, 0b10100010101010010010010010110100, 0b00101011010101011001010101010110));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 2,
	                  U32x8(0b00011011001011010101011010011000, 0b00100010110011011010010110010100, 0b01100110010101001100110000111000, 0b10010101001011001011011001001000, 0b01101000110110101010010110101100, 0b01101010110110010110101101010100, 0b01000101010100100100100101101000, 0b01010110101010110010101010101100));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 3,
	                  U32x8(0b00110110010110101010110100110000, 0b01000101100110110100101100101000, 0b11001100101010011001100001110000, 0b00101010010110010110110010010000, 0b11010001101101010100101101011000, 0b11010101101100101101011010101000, 0b10001010101001001001001011010000, 0b10101101010101100101010101011000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 4,
	                  U32x8(0b01101100101101010101101001100000, 0b10001011001101101001011001010000, 0b10011001010100110011000011100000, 0b01010100101100101101100100100000, 0b10100011011010101001011010110000, 0b10101011011001011010110101010000, 0b00010101010010010010010110100000, 0b01011010101011001010101010110000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 5,
	                  U32x8(0b11011001011010101011010011000000, 0b00010110011011010010110010100000, 0b00110010101001100110000111000000, 0b10101001011001011011001001000000, 0b01000110110101010010110101100000, 0b01010110110010110101101010100000, 0b00101010100100100100101101000000, 0b10110101010110010101010101100000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 6,
	                  U32x8(0b10110010110101010110100110000000, 0b00101100110110100101100101000000, 0b01100101010011001100001110000000, 0b01010010110010110110010010000000, 0b10001101101010100101101011000000, 0b10101101100101101011010101000000, 0b01010101001001001001011010000000, 0b01101010101100101010101011000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 7,
	                  U32x8(0b01100101101010101101001100000000, 0b01011001101101001011001010000000, 0b11001010100110011000011100000000, 0b10100101100101101100100100000000, 0b00011011010101001011010110000000, 0b01011011001011010110101010000000, 0b10101010010010010010110100000000, 0b11010101011001010101010110000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 8,
	                  U32x8(0b11001011010101011010011000000000, 0b10110011011010010110010100000000, 0b10010101001100110000111000000000, 0b01001011001011011001001000000000, 0b00110110101010010110101100000000, 0b10110110010110101101010100000000, 0b01010100100100100101101000000000, 0b10101010110010101010101100000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 9,
	                  U32x8(0b10010110101010110100110000000000, 0b01100110110100101100101000000000, 0b00101010011001100001110000000000, 0b10010110010110110010010000000000, 0b01101101010100101101011000000000, 0b01101100101101011010101000000000, 0b10101001001001001011010000000000, 0b01010101100101010101011000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 10,
	                  U32x8(0b00101101010101101001100000000000, 0b11001101101001011001010000000000, 0b01010100110011000011100000000000, 0b00101100101101100100100000000000, 0b11011010101001011010110000000000, 0b11011001011010110101010000000000, 0b01010010010010010110100000000000, 0b10101011001010101010110000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 11,
	                  U32x8(0b01011010101011010011000000000000, 0b10011011010010110010100000000000, 0b10101001100110000111000000000000, 0b01011001011011001001000000000000, 0b10110101010010110101100000000000, 0b10110010110101101010100000000000, 0b10100100100100101101000000000000, 0b01010110010101010101100000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 12,
	                  U32x8(0b10110101010110100110000000000000, 0b00110110100101100101000000000000, 0b01010011001100001110000000000000, 0b10110010110110010010000000000000, 0b01101010100101101011000000000000, 0b01100101101011010101000000000000, 0b01001001001001011010000000000000, 0b10101100101010101011000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 13,
	                  U32x8(0b01101010101101001100000000000000, 0b01101101001011001010000000000000, 0b10100110011000011100000000000000, 0b01100101101100100100000000000000, 0b11010101001011010110000000000000, 0b11001011010110101010000000000000, 0b10010010010010110100000000000000, 0b01011001010101010110000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 14,
	                  U32x8(0b11010101011010011000000000000000, 0b11011010010110010100000000000000, 0b01001100110000111000000000000000, 0b11001011011001001000000000000000, 0b10101010010110101100000000000000, 0b10010110101101010100000000000000, 0b00100100100101101000000000000000, 0b10110010101010101100000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 15,
	                  U32x8(0b10101010110100110000000000000000, 0b10110100101100101000000000000000, 0b10011001100001110000000000000000, 0b10010110110010010000000000000000, 0b01010100101101011000000000000000, 0b00101101011010101000000000000000, 0b01001001001011010000000000000000, 0b01100101010101011000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 16,
	                  U32x8(0b01010101101001100000000000000000, 0b01101001011001010000000000000000, 0b00110011000011100000000000000000, 0b00101101100100100000000000000000, 0b10101001011010110000000000000000, 0b01011010110101010000000000000000, 0b10010010010110100000000000000000, 0b11001010101010110000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 17,
	                  U32x8(0b10101011010011000000000000000000, 0b11010010110010100000000000000000, 0b01100110000111000000000000000000, 0b01011011001001000000000000000000, 0b01010010110101100000000000000000, 0b10110101101010100000000000000000, 0b00100100101101000000000000000000, 0b10010101010101100000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 18,
	                  U32x8(0b01010110100110000000000000000000, 0b10100101100101000000000000000000, 0b11001100001110000000000000000000, 0b10110110010010000000000000000000, 0b10100101101011000000000000000000, 0b01101011010101000000000000000000, 0b01001001011010000000000000000000, 0b00101010101011000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 19,
	                  U32x8(0b10101101001100000000000000000000, 0b01001011001010000000000000000000, 0b10011000011100000000000000000000, 0b01101100100100000000000000000000, 0b01001011010110000000000000000000, 0b11010110101010000000000000000000, 0b10010010110100000000000000000000, 0b01010101010110000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 20,
	                  U32x8(0b01011010011000000000000000000000, 0b10010110010100000000000000000000, 0b00110000111000000000000000000000, 0b11011001001000000000000000000000, 0b10010110101100000000000000000000, 0b10101101010100000000000000000000, 0b00100101101000000000000000000000, 0b10101010101100000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 21,
	                  U32x8(0b10110100110000000000000000000000, 0b00101100101000000000000000000000, 0b01100001110000000000000000000000, 0b10110010010000000000000000000000, 0b00101101011000000000000000000000, 0b01011010101000000000000000000000, 0b01001011010000000000000000000000, 0b01010101011000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 22,
	                  U32x8(0b01101001100000000000000000000000, 0b01011001010000000000000000000000, 0b11000011100000000000000000000000, 0b01100100100000000000000000000000, 0b01011010110000000000000000000000, 0b10110101010000000000000000000000, 0b10010110100000000000000000000000, 0b10101010110000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 23,
	                  U32x8(0b11010011000000000000000000000000, 0b10110010100000000000000000000000, 0b10000111000000000000000000000000, 0b11001001000000000000000000000000, 0b10110101100000000000000000000000, 0b01101010100000000000000000000000, 0b00101101000000000000000000000000, 0b01010101100000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 24,
	                  U32x8(0b10100110000000000000000000000000, 0b01100101000000000000000000000000, 0b00001110000000000000000000000000, 0b10010010000000000000000000000000, 0b01101011000000000000000000000000, 0b11010101000000000000000000000000, 0b01011010000000000000000000000000, 0b10101011000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 25,
	                  U32x8(0b01001100000000000000000000000000, 0b11001010000000000000000000000000, 0b00011100000000000000000000000000, 0b00100100000000000000000000000000, 0b11010110000000000000000000000000, 0b10101010000000000000000000000000, 0b10110100000000000000000000000000, 0b01010110000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 26,
	                  U32x8(0b10011000000000000000000000000000, 0b10010100000000000000000000000000, 0b00111000000000000000000000000000, 0b01001000000000000000000000000000, 0b10101100000000000000000000000000, 0b01010100000000000000000000000000, 0b01101000000000000000000000000000, 0b10101100000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 27,
	                  U32x8(0b00110000000000000000000000000000, 0b00101000000000000000000000000000, 0b01110000000000000000000000000000, 0b10010000000000000000000000000000, 0b01011000000000000000000000000000, 0b10101000000000000000000000000000, 0b11010000000000000000000000000000, 0b01011000000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 28,
	                  U32x8(0b01100000000000000000000000000000, 0b01010000000000000000000000000000, 0b11100000000000000000000000000000, 0b00100000000000000000000000000000, 0b10110000000000000000000000000000, 0b01010000000000000000000000000000, 0b10100000000000000000000000000000, 0b10110000000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 29,
	                  U32x8(0b11000000000000000000000000000000, 0b10100000000000000000000000000000, 0b11000000000000000000000000000000, 0b01000000000000000000000000000000, 0b01100000000000000000000000000000, 0b10100000000000000000000000000000, 0b01000000000000000000000000000000, 0b01100000000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 30,
	                  U32x8(0b10000000000000000000000000000000, 0b01000000000000000000000000000000, 0b10000000000000000000000000000000, 0b10000000000000000000000000000000, 0b11000000000000000000000000000000, 0b01000000000000000000000000000000, 0b10000000000000000000000000000000, 0b11000000000000000000000000000000));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 31,
	                  U32x8(0b00000000000000000000000000000000, 0b10000000000000000000000000000000, 0b00000000000000000000000000000000, 0b00000000000000000000000000000000, 0b10000000000000000000000000000000, 0b10000000000000000000000000000000, 0b00000000000000000000000000000000, 0b10000000000000000000000000000000));
	ASSERT_CRASH(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) << 32, U"Tried to shift ");
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 0,
	                  U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 1,
	                  U32x8(0b01100011011001011010101011010011, 0b01100100010110011011010010110010, 0b00101100110010101001100110000111, 0b01010010101001011001011011001001, 0b00101101000110110101010010110101, 0b00101101010110110010110101101010, 0b01101000101010100100100100101101, 0b01001010110101010110010101010101));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 2,
	                  U32x8(0b00110001101100101101010101101001, 0b00110010001011001101101001011001, 0b00010110011001010100110011000011, 0b00101001010100101100101101100100, 0b00010110100011011010101001011010, 0b00010110101011011001011010110101, 0b00110100010101010010010010010110, 0b00100101011010101011001010101010));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 3,
	                  U32x8(0b00011000110110010110101010110100, 0b00011001000101100110110100101100, 0b00001011001100101010011001100001, 0b00010100101010010110010110110010, 0b00001011010001101101010100101101, 0b00001011010101101100101101011010, 0b00011010001010101001001001001011, 0b00010010101101010101100101010101));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 4,
	                  U32x8(0b00001100011011001011010101011010, 0b00001100100010110011011010010110, 0b00000101100110010101001100110000, 0b00001010010101001011001011011001, 0b00000101101000110110101010010110, 0b00000101101010110110010110101101, 0b00001101000101010100100100100101, 0b00001001010110101010110010101010));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 5,
	                  U32x8(0b00000110001101100101101010101101, 0b00000110010001011001101101001011, 0b00000010110011001010100110011000, 0b00000101001010100101100101101100, 0b00000010110100011011010101001011, 0b00000010110101011011001011010110, 0b00000110100010101010010010010010, 0b00000100101011010101011001010101));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 6,
	                  U32x8(0b00000011000110110010110101010110, 0b00000011001000101100110110100101, 0b00000001011001100101010011001100, 0b00000010100101010010110010110110, 0b00000001011010001101101010100101, 0b00000001011010101101100101101011, 0b00000011010001010101001001001001, 0b00000010010101101010101100101010));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 7,
	                  U32x8(0b00000001100011011001011010101011, 0b00000001100100010110011011010010, 0b00000000101100110010101001100110, 0b00000001010010101001011001011011, 0b00000000101101000110110101010010, 0b00000000101101010110110010110101, 0b00000001101000101010100100100100, 0b00000001001010110101010110010101));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 8,
	                  U32x8(0b00000000110001101100101101010101, 0b00000000110010001011001101101001, 0b00000000010110011001010100110011, 0b00000000101001010100101100101101, 0b00000000010110100011011010101001, 0b00000000010110101011011001011010, 0b00000000110100010101010010010010, 0b00000000100101011010101011001010));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 9,
	                  U32x8(0b00000000011000110110010110101010, 0b00000000011001000101100110110100, 0b00000000001011001100101010011001, 0b00000000010100101010010110010110, 0b00000000001011010001101101010100, 0b00000000001011010101101100101101, 0b00000000011010001010101001001001, 0b00000000010010101101010101100101));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 10,
	                  U32x8(0b00000000001100011011001011010101, 0b00000000001100100010110011011010, 0b00000000000101100110010101001100, 0b00000000001010010101001011001011, 0b00000000000101101000110110101010, 0b00000000000101101010110110010110, 0b00000000001101000101010100100100, 0b00000000001001010110101010110010));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 11,
	                  U32x8(0b00000000000110001101100101101010, 0b00000000000110010001011001101101, 0b00000000000010110011001010100110, 0b00000000000101001010100101100101, 0b00000000000010110100011011010101, 0b00000000000010110101011011001011, 0b00000000000110100010101010010010, 0b00000000000100101011010101011001));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 12,
	                  U32x8(0b00000000000011000110110010110101, 0b00000000000011001000101100110110, 0b00000000000001011001100101010011, 0b00000000000010100101010010110010, 0b00000000000001011010001101101010, 0b00000000000001011010101101100101, 0b00000000000011010001010101001001, 0b00000000000010010101101010101100));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 13,
	                  U32x8(0b00000000000001100011011001011010, 0b00000000000001100100010110011011, 0b00000000000000101100110010101001, 0b00000000000001010010101001011001, 0b00000000000000101101000110110101, 0b00000000000000101101010110110010, 0b00000000000001101000101010100100, 0b00000000000001001010110101010110));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 14,
	                  U32x8(0b00000000000000110001101100101101, 0b00000000000000110010001011001101, 0b00000000000000010110011001010100, 0b00000000000000101001010100101100, 0b00000000000000010110100011011010, 0b00000000000000010110101011011001, 0b00000000000000110100010101010010, 0b00000000000000100101011010101011));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 15,
	                  U32x8(0b00000000000000011000110110010110, 0b00000000000000011001000101100110, 0b00000000000000001011001100101010, 0b00000000000000010100101010010110, 0b00000000000000001011010001101101, 0b00000000000000001011010101101100, 0b00000000000000011010001010101001, 0b00000000000000010010101101010101));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 16,
	                  U32x8(0b00000000000000001100011011001011, 0b00000000000000001100100010110011, 0b00000000000000000101100110010101, 0b00000000000000001010010101001011, 0b00000000000000000101101000110110, 0b00000000000000000101101010110110, 0b00000000000000001101000101010100, 0b00000000000000001001010110101010));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 17,
	                  U32x8(0b00000000000000000110001101100101, 0b00000000000000000110010001011001, 0b00000000000000000010110011001010, 0b00000000000000000101001010100101, 0b00000000000000000010110100011011, 0b00000000000000000010110101011011, 0b00000000000000000110100010101010, 0b00000000000000000100101011010101));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 18,
	                  U32x8(0b00000000000000000011000110110010, 0b00000000000000000011001000101100, 0b00000000000000000001011001100101, 0b00000000000000000010100101010010, 0b00000000000000000001011010001101, 0b00000000000000000001011010101101, 0b00000000000000000011010001010101, 0b00000000000000000010010101101010));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 19,
	                  U32x8(0b00000000000000000001100011011001, 0b00000000000000000001100100010110, 0b00000000000000000000101100110010, 0b00000000000000000001010010101001, 0b00000000000000000000101101000110, 0b00000000000000000000101101010110, 0b00000000000000000001101000101010, 0b00000000000000000001001010110101));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 20,
	                  U32x8(0b00000000000000000000110001101100, 0b00000000000000000000110010001011, 0b00000000000000000000010110011001, 0b00000000000000000000101001010100, 0b00000000000000000000010110100011, 0b00000000000000000000010110101011, 0b00000000000000000000110100010101, 0b00000000000000000000100101011010));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 21,
	                  U32x8(0b00000000000000000000011000110110, 0b00000000000000000000011001000101, 0b00000000000000000000001011001100, 0b00000000000000000000010100101010, 0b00000000000000000000001011010001, 0b00000000000000000000001011010101, 0b00000000000000000000011010001010, 0b00000000000000000000010010101101));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 22,
	                  U32x8(0b00000000000000000000001100011011, 0b00000000000000000000001100100010, 0b00000000000000000000000101100110, 0b00000000000000000000001010010101, 0b00000000000000000000000101101000, 0b00000000000000000000000101101010, 0b00000000000000000000001101000101, 0b00000000000000000000001001010110));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 23,
	                  U32x8(0b00000000000000000000000110001101, 0b00000000000000000000000110010001, 0b00000000000000000000000010110011, 0b00000000000000000000000101001010, 0b00000000000000000000000010110100, 0b00000000000000000000000010110101, 0b00000000000000000000000110100010, 0b00000000000000000000000100101011));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 24,
	                  U32x8(0b00000000000000000000000011000110, 0b00000000000000000000000011001000, 0b00000000000000000000000001011001, 0b00000000000000000000000010100101, 0b00000000000000000000000001011010, 0b00000000000000000000000001011010, 0b00000000000000000000000011010001, 0b00000000000000000000000010010101));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 25,
	                  U32x8(0b00000000000000000000000001100011, 0b00000000000000000000000001100100, 0b00000000000000000000000000101100, 0b00000000000000000000000001010010, 0b00000000000000000000000000101101, 0b00000000000000000000000000101101, 0b00000000000000000000000001101000, 0b00000000000000000000000001001010));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 26,
	                  U32x8(0b00000000000000000000000000110001, 0b00000000000000000000000000110010, 0b00000000000000000000000000010110, 0b00000000000000000000000000101001, 0b00000000000000000000000000010110, 0b00000000000000000000000000010110, 0b00000000000000000000000000110100, 0b00000000000000000000000000100101));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 27,
	                  U32x8(0b00000000000000000000000000011000, 0b00000000000000000000000000011001, 0b00000000000000000000000000001011, 0b00000000000000000000000000010100, 0b00000000000000000000000000001011, 0b00000000000000000000000000001011, 0b00000000000000000000000000011010, 0b00000000000000000000000000010010));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 28,
	                  U32x8(0b00000000000000000000000000001100, 0b00000000000000000000000000001100, 0b00000000000000000000000000000101, 0b00000000000000000000000000001010, 0b00000000000000000000000000000101, 0b00000000000000000000000000000101, 0b00000000000000000000000000001101, 0b00000000000000000000000000001001));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 29,
	                  U32x8(0b00000000000000000000000000000110, 0b00000000000000000000000000000110, 0b00000000000000000000000000000010, 0b00000000000000000000000000000101, 0b00000000000000000000000000000010, 0b00000000000000000000000000000010, 0b00000000000000000000000000000110, 0b00000000000000000000000000000100));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 30,
	                  U32x8(0b00000000000000000000000000000011, 0b00000000000000000000000000000011, 0b00000000000000000000000000000001, 0b00000000000000000000000000000010, 0b00000000000000000000000000000001, 0b00000000000000000000000000000001, 0b00000000000000000000000000000011, 0b00000000000000000000000000000010));
	ASSERT_EQUAL_SIMD(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 31,
	                  U32x8(0b00000000000000000000000000000001, 0b00000000000000000000000000000001, 0b00000000000000000000000000000000, 0b00000000000000000000000000000001, 0b00000000000000000000000000000000, 0b00000000000000000000000000000000, 0b00000000000000000000000000000001, 0b00000000000000000000000000000001));
	ASSERT_CRASH(U32x8(0b11000110110010110101010110100110, 0b11001000101100110110100101100101, 0b01011001100101010011001100001110, 0b10100101010010110010110110010010, 0b01011010001101101010100101101011, 0b01011010101101100101101011010101, 0b11010001010101001001001001011010, 0b10010101101010101100101010101011) >> 32, U"Tried to shift ");

	// Bit shift with multiple offsets.
	ASSERT_EQUAL_SIMD(U32x4(1, 2, 3, 4) << U32x4(0, 3, 1, 2), U32x4(1, 16, 6, 16));
	ASSERT_EQUAL_SIMD(
	  U32x4(0b11111011111011111111001111101111u, 0b11111111011110011111111110011111u, 0b11111111111011111111101111111101u, 0b11111111011111111101111011111111u) << U32x4(0, 1, 30, 31),
	  U32x4(0b11111011111011111111001111101111u, 0b11111110111100111111111100111110u, 0b01000000000000000000000000000000u, 0b10000000000000000000000000000000u)
	);
	ASSERT_EQUAL_SIMD(
	  U32x4(0b11111011111111110111111100111111u, 0b11111111001111111101101111001111u, 0b11111011111111111111111110111111u, 0b11111111011110111111101111111111u) >> U32x4(0, 1, 30, 31),
	  U32x4(0b11111011111111110111111100111111u, 0b01111111100111111110110111100111u, 0b00000000000000000000000000000011u, 0b00000000000000000000000000000001u)
	);
	ASSERT_EQUAL_SIMD(U32x4(1, 2, 3, 4) << U32x4(2, 4, 3, 1), U32x4(4, 32, 24, 8));
	ASSERT_EQUAL_SIMD(U32x4(64, 32, 5, 8) >> U32x4(2, 1, 2, 0), U32x4(16, 16, 1, 8));
	ASSERT_EQUAL_SIMD(U32x8(1, 2, 3, 4, 5, 6, 7, 8) << U32x8(2, 4, 3, 1, 0, 1, 2, 1), U32x8(4, 32, 24, 8, 5, 12, 28, 16));
	ASSERT_EQUAL_SIMD(U32x8(64, 32, 5, 8, 128, 64, 128, 256) >> U32x8(2, 4, 3, 1, 3, 1, 2, 1), U32x8(16, 2, 0, 4, 16, 32, 32, 128));

	// Bit shift with immediate offset.
	ASSERT_EQUAL_SIMD(bitShiftLeftImmediate<1>(U32x4(1, 2, 3, 4)), U32x4(2, 4, 6, 8));
	ASSERT_EQUAL_SIMD(bitShiftLeftImmediate<2>(U32x4(1, 2, 3, 4)), U32x4(4, 8, 12, 16));
	ASSERT_EQUAL_SIMD(bitShiftLeftImmediate<3>(U32x4(1, 2, 3, 4)), U32x4(8, 16, 24, 32));
	ASSERT_EQUAL_SIMD(bitShiftLeftImmediate<4>(U32x4(1, 2, 3, 4)), U32x4(16, 32, 48, 64));
	ASSERT_EQUAL_SIMD(bitShiftRightImmediate<1>(U32x4(1, 2, 3, 4)), U32x4(0, 1, 1, 2));
	ASSERT_EQUAL_SIMD(bitShiftRightImmediate<1>(U32x4(2, 4, 6, 8)), U32x4(1, 2, 3, 4));
	ASSERT_EQUAL_SIMD(bitShiftRightImmediate<2>(U32x4(2, 4, 6, 8)), U32x4(0, 1, 1, 2));
	ASSERT_EQUAL_SIMD(bitShiftLeftImmediate<4>(U32x4(0x0AB12CD0, 0xFFFFFFFF, 0x12345678, 0xF0000000)), U32x4(0xAB12CD00, 0xFFFFFFF0, 0x23456780, 0x00000000));
	ASSERT_EQUAL_SIMD(bitShiftRightImmediate<4>(U32x4(0x0AB12CD0, 0xFFFFFFFF, 0x12345678, 0x0000000F)), U32x4(0x00AB12CD, 0x0FFFFFFF, 0x01234567, 0x00000000));
	ASSERT_EQUAL_SIMD(bitShiftLeftImmediate <1>(U32x8(1,  2,  3,  4,  5,  6,  7,  8)), U32x8( 2,  4,  6,  8, 10, 12, 14, 16));
	ASSERT_EQUAL_SIMD(bitShiftLeftImmediate <2>(U32x8(1,  2,  3,  4,  5,  6,  7,  8)), U32x8( 4,  8, 12, 16, 20, 24, 28, 32));
	ASSERT_EQUAL_SIMD(bitShiftLeftImmediate <3>(U32x8(1,  2,  3,  4,  5,  6,  7,  8)), U32x8( 8, 16, 24, 32, 40, 48, 56, 64));
	ASSERT_EQUAL_SIMD(bitShiftLeftImmediate <4>(U32x8(1,  2,  3,  4,  5,  6,  7,  8)), U32x8(16, 32, 48, 64, 80, 96,112,128));
	ASSERT_EQUAL_SIMD(bitShiftRightImmediate<1>(U32x8(1,  2,  3,  4,  5,  6,  7,  8)), U32x8( 0,  1,  1,  2,  2,  3,  3,  4));
	ASSERT_EQUAL_SIMD(bitShiftRightImmediate<1>(U32x8(2,  4,  6,  8, 10, 12, 14, 16)), U32x8( 1,  2,  3,  4,  5,  6,  7,  8));
	ASSERT_EQUAL_SIMD(bitShiftRightImmediate<2>(U32x8(2,  4,  6,  8, 10, 12, 14, 16)), U32x8( 0,  1,  1,  2,  2,  3,  3,  4));
	ASSERT_EQUAL_SIMD(
	    bitShiftLeftImmediate<4>(U32x8(0x0AB12CD0, 0xFFFFFFFF, 0x12345678, 0xF0000000, 0x87654321, 0x48484848, 0x76437643, 0x11111111)),
	                             U32x8(0xAB12CD00, 0xFFFFFFF0, 0x23456780, 0x00000000, 0x76543210, 0x84848480, 0x64376430, 0x11111110));
	ASSERT_EQUAL_SIMD(
	    bitShiftRightImmediate<4>(U32x8(0x0AB12CD0, 0xFFFFFFFF, 0x12345678, 0x0000000F, 0x87654321, 0x48484848, 0x76437643, 0x11111111)),
	                              U32x8(0x00AB12CD, 0x0FFFFFFF, 0x01234567, 0x00000000, 0x08765432, 0x04848484, 0x07643764, 0x01111111));
}

static void testVectorExtract() {
	ASSERT_EQUAL_SIMD(vectorExtract_0(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(1, 2, 3, 4));
	ASSERT_EQUAL_SIMD(vectorExtract_1(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(2, 3, 4, 5));
	ASSERT_EQUAL_SIMD(vectorExtract_2(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(3, 4, 5, 6));
	ASSERT_EQUAL_SIMD(vectorExtract_3(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(4, 5, 6, 7));
	ASSERT_EQUAL_SIMD(vectorExtract_4(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(5, 6, 7, 8));
	ASSERT_EQUAL_SIMD(vectorExtract_0(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(123, 4294967295, 712, 45));
	ASSERT_EQUAL_SIMD(vectorExtract_1(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(4294967295, 712, 45, 850514));
	ASSERT_EQUAL_SIMD(vectorExtract_2(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(712, 45, 850514, 27));
	ASSERT_EQUAL_SIMD(vectorExtract_3(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(45, 850514, 27, 0));
	ASSERT_EQUAL_SIMD(vectorExtract_4(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(850514, 27, 0, 174));
	ASSERT_EQUAL_SIMD(vectorExtract_0(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(1, 2, 3, 4));
	ASSERT_EQUAL_SIMD(vectorExtract_1(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(2, 3, 4, 5));
	ASSERT_EQUAL_SIMD(vectorExtract_2(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(3, 4, 5, 6));
	ASSERT_EQUAL_SIMD(vectorExtract_3(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(4, 5, 6, 7));
	ASSERT_EQUAL_SIMD(vectorExtract_4(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(5, 6, 7, 8));
	ASSERT_EQUAL_SIMD(vectorExtract_0(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(123, 8462784, -712, 45));
	ASSERT_EQUAL_SIMD(vectorExtract_1(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(8462784, -712, 45, -37562));
	ASSERT_EQUAL_SIMD(vectorExtract_2(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(-712, 45, -37562, 27));
	ASSERT_EQUAL_SIMD(vectorExtract_3(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(45, -37562, 27, 0));
	ASSERT_EQUAL_SIMD(vectorExtract_4(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(-37562, 27, 0, 174));
	ASSERT_EQUAL_SIMD(vectorExtract_0(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(1.0f, -2.0f, 3.0f, -4.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_1(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(-2.0f, 3.0f, -4.0f, 5.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_2(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(3.0f, -4.0f, 5.0f, 6.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_3(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(-4.0f, 5.0f, 6.0f, -7.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_4(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(5.0f, 6.0f, -7.0f, 8.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_0(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_EQUAL_SIMD(vectorExtract_1(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(2, 3, 4, 5, 6, 7, 8, 9));
	ASSERT_EQUAL_SIMD(vectorExtract_2(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(3, 4, 5, 6, 7, 8, 9, 10));
	ASSERT_EQUAL_SIMD(vectorExtract_3(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(4, 5, 6, 7, 8, 9, 10, 11));
	ASSERT_EQUAL_SIMD(vectorExtract_4(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(5, 6, 7, 8, 9, 10, 11, 12));
	ASSERT_EQUAL_SIMD(vectorExtract_5(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(6, 7, 8, 9, 10, 11, 12, 13));
	ASSERT_EQUAL_SIMD(vectorExtract_6(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(7, 8, 9, 10, 11, 12, 13, 14));
	ASSERT_EQUAL_SIMD(vectorExtract_7(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(8, 9, 10, 11, 12, 13, 14, 15));
	ASSERT_EQUAL_SIMD(vectorExtract_8(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_EQUAL_SIMD(vectorExtract_0(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_EQUAL_SIMD(vectorExtract_1(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17));
	ASSERT_EQUAL_SIMD(vectorExtract_2(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));
	ASSERT_EQUAL_SIMD(vectorExtract_3(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19));
	ASSERT_EQUAL_SIMD(vectorExtract_4(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20));
	ASSERT_EQUAL_SIMD(vectorExtract_5(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21));
	ASSERT_EQUAL_SIMD(vectorExtract_6(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22));
	ASSERT_EQUAL_SIMD(vectorExtract_7(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23));
	ASSERT_EQUAL_SIMD(vectorExtract_8(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24));
	ASSERT_EQUAL_SIMD(vectorExtract_9(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25));
	ASSERT_EQUAL_SIMD(vectorExtract_10(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26));
	ASSERT_EQUAL_SIMD(vectorExtract_11(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27));
	ASSERT_EQUAL_SIMD(vectorExtract_12(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28));
	ASSERT_EQUAL_SIMD(vectorExtract_13(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29));
	ASSERT_EQUAL_SIMD(vectorExtract_14(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30));
	ASSERT_EQUAL_SIMD(vectorExtract_15(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31));
	ASSERT_EQUAL_SIMD(vectorExtract_16(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32));

	ASSERT_EQUAL_SIMD(vectorExtract_0(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                     U32x8( 1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_EQUAL_SIMD(vectorExtract_1(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                        U32x8( 2, 3, 4, 5, 6, 7, 8,         9));
	ASSERT_EQUAL_SIMD(vectorExtract_2(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                           U32x8( 3, 4, 5, 6, 7, 8,         9,10));
	ASSERT_EQUAL_SIMD(vectorExtract_3(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                              U32x8( 4, 5, 6, 7, 8,         9,10,11));
	ASSERT_EQUAL_SIMD(vectorExtract_4(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                                 U32x8( 5, 6, 7, 8,         9,10,11,12));
	ASSERT_EQUAL_SIMD(vectorExtract_5(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                                    U32x8( 6, 7, 8,         9,10,11,12,13));
	ASSERT_EQUAL_SIMD(vectorExtract_6(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                                       U32x8( 7, 8,         9,10,11,12,13,14));
	ASSERT_EQUAL_SIMD(vectorExtract_7(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                                          U32x8( 8,         9,10,11,12,13,14,15));
	ASSERT_EQUAL_SIMD(vectorExtract_8(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                                                     U32x8( 9,10,11,12,13,14,15,16));
	ASSERT_EQUAL_SIMD(vectorExtract_5(U32x8( 1, 2, 3, 4, 5, 6, 7, 4294967295), U32x8( 9,10,11,1000,13,14,15,16)),
	                                                    U32x8( 6, 7, 4294967295,         9,10,11,1000,13));
	ASSERT_EQUAL_SIMD(vectorExtract_0(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                     I32x8( 1,-2, 3, 4,-5, 6, 7, 8));
	ASSERT_EQUAL_SIMD(vectorExtract_1(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                        I32x8(-2, 3, 4,-5, 6, 7, 8,         9));
	ASSERT_EQUAL_SIMD(vectorExtract_2(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                           I32x8( 3, 4,-5, 6, 7, 8,         9,10));
	ASSERT_EQUAL_SIMD(vectorExtract_3(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                              I32x8( 4,-5, 6, 7, 8,         9,10,11));
	ASSERT_EQUAL_SIMD(vectorExtract_4(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                                 I32x8(-5, 6, 7, 8,         9,10,11,-12));
	ASSERT_EQUAL_SIMD(vectorExtract_5(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                                    I32x8( 6, 7, 8,         9,10,11,-12,13));
	ASSERT_EQUAL_SIMD(vectorExtract_6(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                                       I32x8( 7, 8,         9,10,11,-12,13,14));
	ASSERT_EQUAL_SIMD(vectorExtract_7(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                                          I32x8( 8,         9,10,11,-12,13,14,15));
	ASSERT_EQUAL_SIMD(vectorExtract_8(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                                                     I32x8( 9,10,11,-12,13,14,15,-16));
	ASSERT_EQUAL_SIMD(vectorExtract_0(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                  F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_1(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                       F32x8( -2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f,         9.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_2(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                              F32x8( 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f,         9.0f, 10.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_3(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                    F32x8( 4.0f,-5.0f, 6.0f, 7.0f, 8.0f,         9.0f, 10.0f, 11.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_4(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                          F32x8(-5.0f, 6.0f, 7.0f, 8.0f,         9.0f, 10.0f, 11.0f,-12.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_5(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                                F32x8( 6.0f, 7.0f, 8.0f,         9.0f, 10.0f, 11.0f,-12.0f, 13.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_6(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                                      F32x8( 7.0f, 8.0f,         9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_7(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                                            F32x8( 8.0f,         9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_8(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                                                          F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f));
	ASSERT_EQUAL_SIMD(vectorExtract_0 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                   U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16));
	ASSERT_EQUAL_SIMD(vectorExtract_1 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                      U16x16( 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,         17));
	ASSERT_EQUAL_SIMD(vectorExtract_2 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                         U16x16( 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,         17,18));
	ASSERT_EQUAL_SIMD(vectorExtract_3 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                            U16x16( 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,         17,18,19));
	ASSERT_EQUAL_SIMD(vectorExtract_4 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                               U16x16( 5, 6, 7, 8, 9,10,11,12,13,14,15,16,         17,18,19,20));
	ASSERT_EQUAL_SIMD(vectorExtract_5 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                  U16x16( 6, 7, 8, 9,10,11,12,13,14,15,16,         17,18,19,20,21));
	ASSERT_EQUAL_SIMD(vectorExtract_6 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                     U16x16( 7, 8, 9,10,11,12,13,14,15,16,         17,18,19,20,21,22));
	ASSERT_EQUAL_SIMD(vectorExtract_7 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                        U16x16( 8, 9,10,11,12,13,14,15,16,         17,18,19,20,21,22,23));
	ASSERT_EQUAL_SIMD(vectorExtract_8 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                           U16x16( 9,10,11,12,13,14,15,16,         17,18,19,20,21,22,23,24));
	ASSERT_EQUAL_SIMD(vectorExtract_9 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                              U16x16(10,11,12,13,14,15,16,         17,18,19,20,21,22,23,24,25));
	ASSERT_EQUAL_SIMD(vectorExtract_10(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                 U16x16(11,12,13,14,15,16,         17,18,19,20,21,22,23,24,25,26));
	ASSERT_EQUAL_SIMD(vectorExtract_11(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                    U16x16(12,13,14,15,16,         17,18,19,20,21,22,23,24,25,26,27));
	ASSERT_EQUAL_SIMD(vectorExtract_12(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                       U16x16(13,14,15,16,         17,18,19,20,21,22,23,24,25,26,27,28));
	ASSERT_EQUAL_SIMD(vectorExtract_13(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                          U16x16(14,15,16,         17,18,19,20,21,22,23,24,25,26,27,28,29));
	ASSERT_EQUAL_SIMD(vectorExtract_14(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                             U16x16(15,16,         17,18,19,20,21,22,23,24,25,26,27,28,29,30));
	ASSERT_EQUAL_SIMD(vectorExtract_15(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                                U16x16(16,         17,18,19,20,21,22,23,24,25,26,27,28,29,30,31));
	ASSERT_EQUAL_SIMD(vectorExtract_16(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                                            U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32));
	ASSERT_EQUAL_SIMD(vectorExtract_0 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                   U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32));
	ASSERT_EQUAL_SIMD(vectorExtract_1 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                      U8x32( 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33));
	ASSERT_EQUAL_SIMD(vectorExtract_2 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                         U8x32( 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34));
	ASSERT_EQUAL_SIMD(vectorExtract_3 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                            U8x32( 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35));
	ASSERT_EQUAL_SIMD(vectorExtract_4 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                               U8x32( 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36));
	ASSERT_EQUAL_SIMD(vectorExtract_5 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                  U8x32( 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37));
	ASSERT_EQUAL_SIMD(vectorExtract_6 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                     U8x32( 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38));
	ASSERT_EQUAL_SIMD(vectorExtract_7 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                        U8x32( 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39));
	ASSERT_EQUAL_SIMD(vectorExtract_8 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                           U8x32( 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40));
	ASSERT_EQUAL_SIMD(vectorExtract_9 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                              U8x32(10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41));
	ASSERT_EQUAL_SIMD(vectorExtract_10(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                 U8x32(11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42));
	ASSERT_EQUAL_SIMD(vectorExtract_11(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                    U8x32(12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43));
	ASSERT_EQUAL_SIMD(vectorExtract_12(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                       U8x32(13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44));
	ASSERT_EQUAL_SIMD(vectorExtract_13(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                          U8x32(14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45));
	ASSERT_EQUAL_SIMD(vectorExtract_14(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                             U8x32(15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46));
	ASSERT_EQUAL_SIMD(vectorExtract_15(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                U8x32(16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47));
	ASSERT_EQUAL_SIMD(vectorExtract_16(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                   U8x32(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48));
	ASSERT_EQUAL_SIMD(vectorExtract_17(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                      U8x32(18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49));
	ASSERT_EQUAL_SIMD(vectorExtract_18(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                         U8x32(19,20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50));
	ASSERT_EQUAL_SIMD(vectorExtract_19(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                            U8x32(20,21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51));
	ASSERT_EQUAL_SIMD(vectorExtract_20(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                               U8x32(21,22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52));
	ASSERT_EQUAL_SIMD(vectorExtract_21(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                  U8x32(22,23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53));
	ASSERT_EQUAL_SIMD(vectorExtract_22(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                     U8x32(23,24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54));
	ASSERT_EQUAL_SIMD(vectorExtract_23(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                        U8x32(24,25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55));
	ASSERT_EQUAL_SIMD(vectorExtract_24(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                           U8x32(25,26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56));
	ASSERT_EQUAL_SIMD(vectorExtract_25(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                              U8x32(26,27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57));
	ASSERT_EQUAL_SIMD(vectorExtract_26(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                 U8x32(27,28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58));
	ASSERT_EQUAL_SIMD(vectorExtract_27(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                    U8x32(28,29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59));
	ASSERT_EQUAL_SIMD(vectorExtract_28(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                       U8x32(29,30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60));
	ASSERT_EQUAL_SIMD(vectorExtract_29(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                          U8x32(30,31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61));
	ASSERT_EQUAL_SIMD(vectorExtract_30(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                             U8x32(31,32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62));
	ASSERT_EQUAL_SIMD(vectorExtract_31(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                                U8x32(32,        33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63));
	ASSERT_EQUAL_SIMD(vectorExtract_32(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                                           U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64));
}

static void testGather() {
	// The Buffer must be kept alive during the pointer's lifetime to prevent freeing the memory too early with reference counting.
	//   Because SafePointer exists only to be faster than Buffer but safer than a raw pointer.
	Buffer gatherTestBuffer = buffer_create(sizeof(int32_t) * 32);
	{
		// 32-bit floating-point gather
		SafePointer<float> pointerF = buffer_getSafeData<float>(gatherTestBuffer, "float gather test data");
		for (int i = 0; i < 32; i++) { // -32.0f, -30.0f, -28.0f, -26.0f ... 24.0f, 26.0f, 28.0f, 30.0f
			pointerF[i] = i * 2.0f - 32.0f;
		}
		ASSERT_EQUAL_SIMD(gather_F32(pointerF     , U32x4(2, 1, 30, 31)), F32x4(-28.0f, -30.0f, 28.0f, 30.0f));
		ASSERT_EQUAL_SIMD(gather_F32(pointerF + 10, U32x4(0, 1, 2, 3)), F32x4(-12.0f, -10.0f, -8.0f, -6.0f));
		ASSERT_EQUAL_SIMD(gather_F32(pointerF     , U32x8(2, 1, 28, 29, 3, 0, 30, 31)), F32x8(-28.0f, -30.0f, 24.0f, 26.0f, -26.0f, -32.0f, 28.0f, 30.0f));
		ASSERT_EQUAL_SIMD(gather_F32(pointerF + 10, U32x8(0, 1, 2, 3, 4, 5, 6, 7)), F32x8(-12.0f, -10.0f, -8.0f, -6.0f, -4.0f, -2.0f, 0.0f, 2.0f));
	}
	{
		// Signed 32-bit integer gather
		SafePointer<int32_t> pointerU = buffer_getSafeData<int32_t>(gatherTestBuffer, "int32_t gather test data");
		for (int i = 0; i < 32; i++) { // -32, -30, -28, -26 ... 24, 26, 28, 30
			pointerU[i] = i * 2 - 32;
		}
		ASSERT_EQUAL_SIMD(gather_I32(pointerU     , U32x4(2, 1, 30, 31)), I32x4(-28, -30, 28, 30));
		ASSERT_EQUAL_SIMD(gather_I32(pointerU + 10, U32x4(0, 1, 2, 3)), I32x4(-12, -10, -8, -6));
		ASSERT_EQUAL_SIMD(gather_I32(pointerU     , U32x8(2, 1, 28, 29, 3, 0, 30, 31)), I32x8(-28, -30, 24, 26, -26, -32, 28, 30));
		ASSERT_EQUAL_SIMD(gather_I32(pointerU + 10, U32x8(0, 1, 2, 3, 4, 5, 6, 7)), I32x8(-12, -10, -8, -6, -4, -2, 0, 2));
	}
	{
		// Unsigned 32-bit integer gather
		SafePointer<uint32_t> pointerI = buffer_getSafeData<uint32_t>(gatherTestBuffer, "uint32_t gather test data");
		for (int i = 0; i < 32; i++) { // 100, 102, 104, 106 ... 156, 158, 160, 162
			pointerI[i] = 100 + i * 2;
		}
		// Signed 32-bit integer gather
		ASSERT_EQUAL_SIMD(gather_U32(pointerI     , U32x4(2, 1, 30, 31)), U32x4(104, 102, 160, 162));
		ASSERT_EQUAL_SIMD(gather_U32(pointerI + 10, U32x4(0, 1, 2, 3)), U32x4(120, 122, 124, 126));
		ASSERT_EQUAL_SIMD(gather_U32(pointerI     , U32x8(2, 1, 28, 29, 3, 0, 30, 31)), U32x8(104, 102, 156, 158, 106, 100, 160, 162));
		ASSERT_EQUAL_SIMD(gather_U32(pointerI + 10, U32x8(0, 1, 2, 3, 4, 5, 6, 7)), U32x8(120, 122, 124, 126, 128, 130, 132, 134));
	}
}

START_TEST(Simd)
	printText(U"\nThe SIMD test is compiled using:\n");
	#ifdef USE_SSE2
		printText(U"	* SSE2\n");
	#endif
	#ifdef USE_SSSE3
		printText(U"	* SSSE3\n");
	#endif
	#ifdef USE_AVX
		printText(U"	* AVX\n");
	#endif
	#ifdef USE_AVX2
		printText(U"	* AVX2\n");
	#endif
	#ifdef USE_NEON
		printText(U"	* NEON\n");
	#endif

	testComparisons();

	// Reciprocal: 1 / x
	ASSERT_EQUAL_SIMD(reciprocal(F32x4(0.5f, 1.0f, 2.0f, 4.0f)), F32x4(2.0f, 1.0f, 0.5f, 0.25f));
	ASSERT_EQUAL_SIMD(reciprocal(F32x8(0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 10.0f, 100.0f, 1000.0f)), F32x8(2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.1f, 0.01f, 0.001f));

	// Reciprocal square root: 1 / sqrt(x)
	ASSERT_EQUAL_SIMD(reciprocalSquareRoot(F32x4(1.0f, 4.0f, 16.0f, 100.0f)), F32x4(1.0f, 0.5f, 0.25f, 0.1f));
	ASSERT_EQUAL_SIMD(reciprocalSquareRoot(F32x8(1.0f, 4.0f, 16.0f, 100.0f, 400.0f, 64.0f, 25.0f, 100.0f)), F32x8(1.0f, 0.5f, 0.25f, 0.1f, 0.05f, 0.125f, 0.2f, 0.1f));

	// Square root: sqrt(x)
	ASSERT_EQUAL_SIMD(squareRoot(F32x4(1.0f, 4.0f, 9.0f, 100.0f)), F32x4(1.0f, 2.0f, 3.0f, 10.0f));
	ASSERT_EQUAL_SIMD(squareRoot(F32x8(1.0f, 4.0f, 9.0f, 100.0f, 64.0f, 256.0f, 1024.0f, 4096.0f)), F32x8(1.0f, 2.0f, 3.0f, 10.0f, 8.0f, 16.0f, 32.0f, 64.0f));

	// Minimum
	ASSERT_EQUAL_SIMD(min(F32x4(1.1f, 2.2f, 3.3f, 4.4f), F32x4(5.0f, 3.0f, 1.0f, -1.0f)), F32x4(1.1f, 2.2f, 1.0f, -1.0f));
	ASSERT_EQUAL_SIMD(min(F32x8(1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 8.8f), F32x8(5.0f, 3.0f, 1.0f, -1.0f, 4.0f, 5.0f, -2.5f, 10.0f)), F32x8(1.1f, 2.2f, 1.0f, -1.0f, 4.0f, 5.0f, -2.5f, 8.8f));

	// Maximum
	ASSERT_EQUAL_SIMD(max(F32x4(1.1f, 2.2f, 3.3f, 4.4f), F32x4(5.0f, 3.0f, 1.0f, -1.0f)), F32x4(5.0f, 3.0f, 3.3f, 4.4f));
	ASSERT_EQUAL_SIMD(max(F32x8(1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 8.8f), F32x8(5.0f, 3.0f, 1.0f, -1.0f, 4.0f, 5.0f, -2.5f, 10.0f)), F32x8(5.0f, 3.0f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 10.0f));

	// Absolute
	ASSERT_EQUAL_SIMD(
	  abs(F32x4(1.1f,-2.2f, 3.3f,-4.4f)),
	      F32x4(1.1f, 2.2f, 3.3f, 4.4f)
	);
	ASSERT_EQUAL_SIMD(
	  abs(F32x8(1.1f,-2.2f,-3.3f, 4.4f, 5.5f,-6.6f,-7.7f,-8.8f)),
	      F32x8(1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 8.8f)
	);
	ASSERT_EQUAL_SIMD(
	  abs(I32x4(1,-2, 3,-4)),
	      I32x4(1, 2, 3, 4)
	);
	ASSERT_EQUAL_SIMD(
	  abs(I32x8(1,-2,-3, 4, 5,-6,-7,-8)),
	      I32x8(1, 2, 3, 4, 5, 6, 7, 8)
	);

	// Clamp
	ASSERT_EQUAL_SIMD(clamp(F32x4(-1.5f), F32x4(-35.1f, 1.0f, 2.0f, 45.7f), F32x4(1.5f)), F32x4(-1.5f, 1.0f, 1.5f, 1.5f));
	ASSERT_EQUAL_SIMD(clampUpper(F32x4(-35.1f, 1.0f, 2.0f, 45.7f), F32x4(1.5f)), F32x4(-35.1f, 1.0f, 1.5f, 1.5f));
	ASSERT_EQUAL_SIMD(clampLower(F32x4(-1.5f), F32x4(-35.1f, 1.0f, 2.0f, 45.7f)), F32x4(-1.5f, 1.0f, 2.0f, 45.7f));
	ASSERT_EQUAL_SIMD(clamp(F32x8(-1.5f), F32x8(-35.1f, 1.0f, 2.0f, 45.7f, 0.0f, -1.0f, 2.1f, -1.9f), F32x8(1.5f)), F32x8(-1.5f, 1.0f, 1.5f, 1.5f, 0.0f, -1.0f, 1.5f, -1.5f));
	ASSERT_EQUAL_SIMD(clampUpper(F32x8(-35.1f, 1.0f, 2.0f, 45.7f, 0.0f, -1.0f, 2.1f, -1.9f), F32x8(1.5f)), F32x8(-35.1f, 1.0f, 1.5f, 1.5f, 0.0f, -1.0f, 1.5f, -1.9f));
	ASSERT_EQUAL_SIMD(clampLower(F32x8(-1.5f), F32x8(-35.1f, 1.0f, 2.0f, 45.7f, 0.0f, -1.0f, 2.1f, -1.9f)), F32x8(-1.5f, 1.0f, 2.0f, 45.7f, 0.0f, -1.0f, 2.1f, -1.5f));

	// Float to integer conversions
	// Underflow and overflow is undefined behavior, because NEON will clamp out of bound values while SSE will truncate away higher bits.
	ASSERT_EQUAL_SIMD(truncateToU32(F32x4(0.01f, 0.99f, 1.01f, 1.99f)),U32x4(0, 0, 1, 1));
	ASSERT_EQUAL_SIMD(truncateToI32(F32x4(0.01f, 0.99f, 1.01f, 1.99f)),I32x4(0, 0, 1, 1));
	ASSERT_EQUAL_SIMD(truncateToI32(F32x4(-0.01f, -0.99f, -1.01f, -1.99f)),I32x4(0, 0, -1, -1));
	ASSERT_EQUAL_SIMD(truncateToU32(F32x4(0.1f, 5.4f, 2.6f, 4.9f)),U32x4(0, 5, 2, 4));
	ASSERT_EQUAL_SIMD(truncateToI32(F32x4(0.1f, 5.4f, 2.6f, 4.9f)),I32x4(0, 5, 2, 4));
	ASSERT_EQUAL_SIMD(truncateToI32(F32x4(-1.1f, -0.9f, -0.1f, 0.1f)),I32x4(-1, 0, 0, 0));
	ASSERT_EQUAL_SIMD(truncateToI32(F32x4(-1000.9f, -23.4f, 123456.7f, 846.999f)),I32x4(-1000, -23, 123456, 846));

	// F32x4 operations
	ASSERT_EQUAL_SIMD(F32x4(1.1f, -2.2f, 3.3f, 4.0f) + F32x4(2.2f, -4.4f, 6.6f, 8.0f), F32x4(3.3f, -6.6f, 9.9f, 12.0f));
	ASSERT_EQUAL_SIMD(F32x4(-1.5f, -0.5f, 0.5f, 1.5f) + 1.0f, F32x4(-0.5f, 0.5f, 1.5f, 2.5f));
	ASSERT_EQUAL_SIMD(1.0f + F32x4(-1.5f, -0.5f, 0.5f, 1.5f), F32x4(-0.5f, 0.5f, 1.5f, 2.5f));
	ASSERT_EQUAL_SIMD(F32x4(1.1f, 2.2f, 3.3f, 4.4f) - F32x4(0.1f, 0.2f, 0.3f, 0.4f), F32x4(1.0f, 2.0f, 3.0f, 4.0f));
	ASSERT_EQUAL_SIMD(F32x4(1.0f, 2.0f, 3.0f, 4.0f) - 0.5f, F32x4(0.5f, 1.5f, 2.5f, 3.5f));
	ASSERT_EQUAL_SIMD(0.5f - F32x4(1.0f, 2.0f, 3.0f, 4.0f), F32x4(-0.5f, -1.5f, -2.5f, -3.5f));
	ASSERT_EQUAL_SIMD(2.0f * F32x4(1.0f, 2.0f, 3.0f, 4.0f), F32x4(2.0f, 4.0f, 6.0f, 8.0f));
	ASSERT_EQUAL_SIMD(F32x4(1.0f, -2.0f, 3.0f, -4.0f) * -2.0f, F32x4(-2.0f, 4.0f, -6.0f, 8.0f));
	ASSERT_EQUAL_SIMD(F32x4(1.0f, -2.0f, 3.0f, -4.0f) * F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(1.0f, 4.0f, 9.0f, 16.0f));
	ASSERT_EQUAL_SIMD(-F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(-1.0f, 2.0f, -3.0f, 4.0f));

	// F32x8 operations
	ASSERT_EQUAL_SIMD(F32x8(1.1f, -2.2f, 3.3f, 4.0f, 1.4f, 2.3f, 3.2f, 4.1f) + F32x8(2.2f, -4.4f, 6.6f, 8.0f, 4.11f, 3.22f, 2.33f, 1.44f), F32x8(3.3f, -6.6f, 9.9f, 12.0f, 5.51f, 5.52f, 5.53f, 5.54f));
	ASSERT_EQUAL_SIMD(F32x8(-1.5f, -0.5f, 0.5f, 1.5f, 1000.0f, 2000.0f, -4000.0f, -1500.0f) + 1.0f, F32x8(-0.5f, 0.5f, 1.5f, 2.5f, 1001.0f, 2001.0f, -3999.0f, -1499.0f));
	ASSERT_EQUAL_SIMD(1.0f + F32x8(-1.5f, -0.5f, 0.5f, 1.5f, 1000.0f, 2000.0f, -4000.0f, -1500.0f), F32x8(-0.5f, 0.5f, 1.5f, 2.5f, 1001.0f, 2001.0f, -3999.0f, -1499.0f));
	ASSERT_EQUAL_SIMD(F32x8(1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 8.8f) - F32x8(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f), F32x8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f));
	ASSERT_EQUAL_SIMD(F32x8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f) - 0.5f, F32x8(0.5f, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f));
	ASSERT_EQUAL_SIMD(0.5f - F32x8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f), F32x8(-0.5f, -1.5f, -2.5f, -3.5f, -4.5f, -5.5f, -6.5f, -7.5f));
	ASSERT_EQUAL_SIMD(2.0f * F32x8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f), F32x8(2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f));
	ASSERT_EQUAL_SIMD(F32x8(1.0f, -2.0f, 3.0f, -4.0f, 5.0f, -6.0f, 7.0f, -8.0f) * -2.0f, F32x8(-2.0f, 4.0f, -6.0f, 8.0f, -10.0f, 12.0f, -14.0f, 16.0f));
	ASSERT_EQUAL_SIMD(F32x8(1.0f, -2.0f, 3.0f, -4.0f, 5.0f, -6.0f, 7.0f, -8.0f) * F32x8(1.0f, -2.0f, 3.0f, -4.0f, 5.0f, -6.0f, 7.0f, -8.0f), F32x8(1.0f, 4.0f, 9.0f, 16.0f, 25.0f, 36.0f, 49.0f, 64.0f));
	ASSERT_EQUAL_SIMD(-F32x8(1.0f, -2.0f, 3.0f, -4.0f, 5.0f, -6.0f, 7.0f, -8.0f), F32x8(-1.0f, 2.0f, -3.0f, 4.0f, -5.0f, 6.0f, -7.0f, 8.0f));

	// I32x4 operations
	ASSERT_EQUAL_SIMD(I32x4(1, 2, -3, 4) + I32x4(-2, 4, 6, 8), I32x4(-1, 6, 3, 12));
	ASSERT_EQUAL_SIMD(I32x4(1, -2, 3, 4) - 4, I32x4(-3, -6, -1, 0));
	ASSERT_EQUAL_SIMD(10 + I32x4(1, 2, 3, 4), I32x4(11, 12, 13, 14));
	ASSERT_EQUAL_SIMD(I32x4(1, 2, 3, 4) + I32x4(4), I32x4(5, 6, 7, 8));
	ASSERT_EQUAL_SIMD(I32x4(10) + I32x4(1, 2, 3, 4), I32x4(11, 12, 13, 14));
	ASSERT_EQUAL_SIMD(I32x4(-3, 6, -9, 12) * I32x4(1, 2, -3, -4), I32x4(-3, 12, 27, -48));
	ASSERT_EQUAL_SIMD(-I32x4(1, -2, 3, -4), I32x4(-1, 2, -3, 4));

	// I32x8 operations
	ASSERT_EQUAL_SIMD(I32x8(1, 2, 3, 4, 5, 6, 7, 8) - 1, I32x8(0, 1, 2, 3, 4, 5, 6, 7));
	ASSERT_EQUAL_SIMD(1 - I32x8(1, 2, 3, 4, 5, 6, 7, 8), I32x8(0, -1, -2, -3, -4, -5, -6, -7));
	ASSERT_EQUAL_SIMD(2 * I32x8(1, 2, 3, 4, 5, 6, 7, 8), I32x8(2, 4, 6, 8, 10, 12, 14, 16));
	ASSERT_EQUAL_SIMD(I32x8(1, -2, 3, -4, 5, -6, 7, -8) * -2, I32x8(-2, 4, -6, 8, -10, 12, -14, 16));
	ASSERT_EQUAL_SIMD(I32x8(1, -2, 3, -4, 5, -6, 7, -8) * I32x8(1, -2, 3, -4, 5, -6, 7, -8), I32x8(1, 4, 9, 16, 25, 36, 49, 64));
	ASSERT_EQUAL_SIMD(-I32x8(1, -2, 3, -4, 5, -6, 7, -8), I32x8(-1, 2, -3, 4, -5, 6, -7, 8));

	// U32x4 operations
	ASSERT_EQUAL_SIMD(U32x4(1, 2, 3, 4) + U32x4(2, 4, 6, 8), U32x4(3, 6, 9, 12));
	ASSERT_EQUAL_SIMD(U32x4(1, 2, 3, 4) + 4, U32x4(5, 6, 7, 8));
	ASSERT_EQUAL_SIMD(10 + U32x4(1, 2, 3, 4), U32x4(11, 12, 13, 14));
	ASSERT_EQUAL_SIMD(U32x4(1, 2, 3, 4) + U32x4(4), U32x4(5, 6, 7, 8));
	ASSERT_EQUAL_SIMD(U32x4(10) + U32x4(1, 2, 3, 4), U32x4(11, 12, 13, 14));
	ASSERT_EQUAL_SIMD(U32x4(3, 6, 9, 12) - U32x4(1, 2, 3, 4), U32x4(2, 4, 6, 8));
	ASSERT_EQUAL_SIMD(U32x4(3, 6, 9, 12) * U32x4(1, 2, 3, 4), U32x4(3, 12, 27, 48));

	// U32x8 operations
	ASSERT_EQUAL_SIMD(U32x8(1, 2, 3, 4, 5, 6, 7, 8) - 1, U32x8(0, 1, 2, 3, 4, 5, 6, 7));
	ASSERT_EQUAL_SIMD(10 - U32x8(1, 2, 3, 4, 5, 6, 7, 8), U32x8(9, 8, 7, 6, 5, 4, 3, 2));
	ASSERT_EQUAL_SIMD(2 * U32x8(1, 2, 3, 4, 5, 6, 7, 8), U32x8(2, 4, 6, 8, 10, 12, 14, 16));
	ASSERT_EQUAL_SIMD(U32x8(1, 2, 3, 4, 5, 6, 7, 8) * 2, U32x8(2, 4, 6, 8, 10, 12, 14, 16));
	ASSERT_EQUAL_SIMD(U32x8(1, 2, 3, 4, 5, 6, 7, 8) * U32x8(1, 2, 3, 4, 5, 6, 7, 8), U32x8(1, 4, 9, 16, 25, 36, 49, 64));

	// U16x8 operations
	ASSERT_EQUAL_SIMD(U16x8(1, 2, 3, 4, 5, 6, 7, 8) + U16x8(2, 4, 6, 8, 10, 12, 14, 16), U16x8(3, 6, 9, 12, 15, 18, 21, 24));
	ASSERT_EQUAL_SIMD(U16x8(1, 2, 3, 4, 5, 6, 7, 8) + 8, U16x8(9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_EQUAL_SIMD(10 + U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(11, 12, 13, 14, 15, 16, 17, 18));
	ASSERT_EQUAL_SIMD(U16x8(1, 2, 3, 4, 5, 6, 7, 8) + U16x8((uint16_t)8), U16x8(9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_EQUAL_SIMD(U16x8((uint16_t)10) + U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(11, 12, 13, 14, 15, 16, 17, 18));
	ASSERT_EQUAL_SIMD(U16x8(3, 6, 9, 12, 15, 18, 21, 24) - U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(2, 4, 6, 8, 10, 12, 14, 16));

	// U16x16 operations
	ASSERT_EQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) + U16x16(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32), U16x16(3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48));
	ASSERT_EQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) + 8, U16x16(9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24));
	ASSERT_EQUAL_SIMD(8 + U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24));
	ASSERT_EQUAL_SIMD(U16x16(3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48) - U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32));
	ASSERT_EQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) - 1, U16x16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
	ASSERT_EQUAL_SIMD(16 - U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0));
	ASSERT_EQUAL_SIMD(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) * 2, U16x16(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32));
	ASSERT_EQUAL_SIMD(2 * U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32));

	// U8x16 operations
	ASSERT_EQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) + 2, U8x16(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));
	ASSERT_EQUAL_SIMD(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) - 1, U8x16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
	ASSERT_EQUAL_SIMD(
	  saturatedAddition(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 255), U8x16((uint8_t)250)),
	  U8x16(251, 252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255)
	);
	ASSERT_EQUAL_SIMD(
	  saturatedSubtraction(
	  U8x16(128, 128, 128, 0, 255, 255,   0, 200, 123, 80, 46, 46, 46, 255, 255, 255),
	  U8x16(  0, 128, 255, 0, 255,   0, 255, 100,  23, 81, 45, 46, 47, 128, 127, 200)),
	  U8x16(128,   0,   0, 0,   0, 255,   0, 100, 100,  0,  1,  0,  0, 127, 128,  55)
	);

	// U8x32 operations
	ASSERT_EQUAL_SIMD(
	      U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)
	    + U8x32( 2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64),
	      U8x32( 3,  6,  9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93, 96));
	ASSERT_EQUAL_SIMD(
	      U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32) + 5,
	      U8x32( 6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37));
	ASSERT_EQUAL_SIMD(
	  5 + U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32),
	      U8x32( 6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37));
	ASSERT_EQUAL_SIMD(
	      U8x32( 3,  6,  9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93, 96)
	    - U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32),
	      U8x32( 2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64));
	ASSERT_EQUAL_SIMD(
	      U8x32( 6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37) - 5,
	      U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32));
	ASSERT_EQUAL_SIMD(
	 33 - U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32),
	      U8x32(32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1));
	ASSERT_EQUAL_SIMD(
	  saturatedAddition(
	    U8x32(  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,255),
	    U8x32((uint8_t)240)),
	    U8x32(241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255)
	);
	ASSERT_EQUAL_SIMD(
	  saturatedSubtraction(
	    U8x32(  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,255),
	    U8x32((uint8_t)16)),
	    U8x32(  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,239)
	);

	// Unsigned integer unpacking
	ASSERT_EQUAL_SIMD(lowerToU32(U16x8(1,2,3,4,5,6,7,8)), U32x4(1, 2, 3, 4));
	ASSERT_EQUAL_SIMD(higherToU32(U16x8(1,2,3,4,5,6,7,8)), U32x4(5, 6, 7, 8));
	ASSERT_EQUAL_SIMD(lowerToU16(U8x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)), U16x8(1,2,3,4,5,6,7,8));
	ASSERT_EQUAL_SIMD(higherToU16(U8x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)), U16x8(9,10,11,12,13,14,15,16));
	ASSERT_EQUAL_SIMD(lowerToU32(U16x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)), U32x8(1,2,3,4,5,6,7,8));
	ASSERT_EQUAL_SIMD(higherToU32(U16x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)), U32x8(9,10,11,12,13,14,15,16));
	ASSERT_EQUAL_SIMD(lowerToU32(U16x16(1,2,3,4,5,6,65535,8,9,10,11,12,13,1000,15,16)), U32x8(1,2,3,4,5,6,65535,8));
	ASSERT_EQUAL_SIMD(higherToU32(U16x16(1,2,3,4,5,6,65535,8,9,10,11,12,13,1000,15,16)), U32x8(9,10,11,12,13,1000,15,16));
	ASSERT_EQUAL_SIMD(lowerToU16(U8x32(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,255,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,255)), U16x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,255));
	ASSERT_EQUAL_SIMD(higherToU16(U8x32(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,255,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,255)), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,255));

	testBitMasks();

	testBitShift();

	// Bitwise negation.
	ASSERT_EQUAL_SIMD(
	  ~U32x4(0b11000000111000000111111100001100, 0b00111000000000110000001110001100, 0b00001110000000100011000000011001, 0b00001110001000000111001100001000),
	   U32x4(0b00111111000111111000000011110011, 0b11000111111111001111110001110011, 0b11110001111111011100111111100110, 0b11110001110111111000110011110111)
	);
	ASSERT_EQUAL_SIMD(
	  ~U16x8(0b1100000011100000, 0b0111111100001100, 0b0011100000000011, 0b0000001110001100, 0b0000111000000010, 0b0011000000011000, 0b0000111000100000, 0b0111001100001000),
	   U16x8(0b0011111100011111, 0b1000000011110011, 0b1100011111111100, 0b1111110001110011, 0b1111000111111101, 0b1100111111100111, 0b1111000111011111, 0b1000110011110111)
	);
	ASSERT_EQUAL_SIMD(
	  ~U32x8(0b11000000111000000111111100001100, 0b00111000000000110000001110001100, 0b00001110000000100011000000011000, 0b00001110001000000111001100001000, 0b11000000111000100111101100101100, 0b00111010000000110010001110101101, 0b01001110001000100011001000010010, 0b01001110001001000111100110000100),
	   U32x8(0b00111111000111111000000011110011, 0b11000111111111001111110001110011, 0b11110001111111011100111111100111, 0b11110001110111111000110011110111, 0b00111111000111011000010011010011, 0b11000101111111001101110001010010, 0b10110001110111011100110111101101, 0b10110001110110111000011001111011)
	);
	ASSERT_EQUAL_SIMD(
	  ~U16x16(0b1100000011100000, 0b0111111100001100, 0b0011100000000011, 0b0000001110001100, 0b0000111000000010, 0b0011000000011000, 0b0000111000100000, 0b0111001100001000,  0b1100100011100100, 0b0110011100001110, 0b0010100001001011, 0b0001001110001110, 0b0000111011000110, 0b0011000111011000, 0b0000111000100100, 0b0101001100011000),
	   U16x16(0b0011111100011111, 0b1000000011110011, 0b1100011111111100, 0b1111110001110011, 0b1111000111111101, 0b1100111111100111, 0b1111000111011111, 0b1000110011110111,  0b0011011100011011, 0b1001100011110001, 0b1101011110110100, 0b1110110001110001, 0b1111000100111001, 0b1100111000100111, 0b1111000111011011, 0b1010110011100111)
	);

	// Reinterpret cast.
	ASSERT_EQUAL_SIMD(
	  reinterpret_U8FromU32(U32x4(ENDIAN32_BYTE_0, ENDIAN32_BYTE_1, ENDIAN32_BYTE_2, ENDIAN32_BYTE_3)),
	  U8x16(
		255, 0, 0, 0,
		0, 255, 0, 0,
		0, 0, 255, 0,
		0, 0, 0, 255
	  )
	);
	ASSERT_EQUAL_SIMD(
	  reinterpret_U8FromU32(U32x4(
		ENDIAN32_BYTE_0 | ENDIAN32_BYTE_2,
		ENDIAN32_BYTE_0 | ENDIAN32_BYTE_3,
		ENDIAN32_BYTE_1,
		ENDIAN32_BYTE_1 | ENDIAN32_BYTE_3
	  )),
	  U8x16(
		255, 0, 255, 0,
		255, 0, 0, 255,
		0, 255, 0, 0,
		0, 255, 0, 255
	  )
	);
	ASSERT_EQUAL_SIMD(
	  reinterpret_U32FromU8(U8x16(
		255, 0, 255, 0,
		255, 0, 0, 255,
		0, 255, 0, 0,
		0, 255, 0, 255
	  )),
	  U32x4(
		ENDIAN32_BYTE_0 | ENDIAN32_BYTE_2,
		ENDIAN32_BYTE_0 | ENDIAN32_BYTE_3,
		ENDIAN32_BYTE_1,
		ENDIAN32_BYTE_1 | ENDIAN32_BYTE_3
	  )
	);
	#ifdef DSR_BIG_ENDIAN
		ASSERT_EQUAL_SIMD(
		  reinterpret_U32FromU16(U16x8(1, 2, 3, 4, 5, 6, 7, 8)),
		  U32x4(1 * 65536 + 2, 3 * 65536 + 4, 5 * 65536 + 6, 7 * 65536 + 8)
		);
		ASSERT_EQUAL_SIMD(
		  reinterpret_U32FromU16(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)),
		  U32x8(1 * 65536 + 2, 3 * 65536 + 4, 5 * 65536 + 6, 7 * 65536 + 8, 9 * 65536 + 10, 11 * 65536 + 12, 13 * 65536 + 14, 15 * 65536 + 16)
		);
		ASSERT_EQUAL_SIMD(
		  reinterpret_U16FromU32(U32x4(1 * 65536 + 2, 3 * 65536 + 4, 5 * 65536 + 6, 7 * 65536 + 8)),
		  U16x8(1, 2, 3, 4, 5, 6, 7, 8)
		);
		ASSERT_EQUAL_SIMD(
		  reinterpret_U16FromU32(U32x8(1 * 65536 + 2, 3 * 65536 + 4, 5 * 65536 + 6, 7 * 65536 + 8, 9 * 65536 + 10, 11 * 65536 + 12, 13 * 65536 + 14, 15 * 65536 + 16)),
		  U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)
		);
	#else
		ASSERT_EQUAL_SIMD(
		  reinterpret_U32FromU16(U16x8(1, 2, 3, 4, 5, 6, 7, 8)),
		  U32x4(1 + 2 * 65536, 3 + 4 * 65536, 5 + 6 * 65536, 7 + 8 * 65536)
		);
		ASSERT_EQUAL_SIMD(
		  reinterpret_U32FromU16(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)),
		  U32x8(1 + 2 * 65536, 3 + 4 * 65536, 5 + 6 * 65536, 7 + 8 * 65536, 9 + 10 * 65536, 11 + 12 * 65536, 13 + 14 * 65536, 15 + 16 * 65536)
		);
		ASSERT_EQUAL_SIMD(
		  reinterpret_U16FromU32(U32x4(1 + 2 * 65536, 3 + 4 * 65536, 5 + 6 * 65536, 7 + 8 * 65536)),
		  U16x8(1, 2, 3, 4, 5, 6, 7, 8)
		);
		ASSERT_EQUAL_SIMD(
		  reinterpret_U16FromU32(U32x8(1 + 2 * 65536, 3 + 4 * 65536, 5 + 6 * 65536, 7 + 8 * 65536, 9 + 10 * 65536, 11 + 12 * 65536, 13 + 14 * 65536, 15 + 16 * 65536)),
		  U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)
		);
	#endif

	testVectorExtract();

	testGather();

END_TEST
