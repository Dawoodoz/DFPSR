
#include "../testTools.h"
#include "../../DFPSR/base/simd.h"

// TODO: Test: allLanesNotEqual, allLanesLesser, allLanesGreater, allLanesLesserOrEqual, allLanesGreaterOrEqual, reinterpret_U16FromU32, reinterpret_U32FromU16, operand ~

START_TEST(Simd)
	printText("\nSIMD test is compiled using:\n");
	#ifdef USE_SSE2
		printText("	* SSE2\n");
	#endif
	#ifdef USE_SSSE3
		printText("	* SSSE3\n");
	#endif
	#ifdef USE_AVX
		printText("	* AVX\n");
	#endif
	#ifdef USE_AVX2
		printText("	* AVX2\n");
	#endif
	#ifdef USE_NEON
		printText("	* NEON\n");
	#endif

	// F32x4 Comparisons
	ASSERT(allLanesEqual(F32x4(1.5f), F32x4(1.5f, 1.5f, 1.5f, 1.5f)));
	ASSERT(allLanesEqual(F32x4(-1.5f), F32x4(-1.5f, -1.5f, -1.5f, -1.5f)));
	ASSERT(allLanesEqual(F32x4(1.2f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, 7.8f)));
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().x, 1.2f);
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().y, 3.4f);
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().z, 5.6f);
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().w, 7.8f);
	ASSERT(!allLanesEqual(F32x4(1.3f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, 7.8f)));
	ASSERT(!allLanesEqual(F32x4(1.2f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, -1.4f, 5.6f, 7.8f)));
	ASSERT(!allLanesEqual(F32x4(1.2f, 3.4f, 5.5f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, 7.8f)));
	ASSERT(!allLanesEqual(F32x4(1.2f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, -7.8f)));

	// I32x4 Comparisons
	ASSERT(allLanesEqual(I32x4(4), I32x4(4, 4, 4, 4)));
	ASSERT(allLanesEqual(I32x4(-4), I32x4(-4, -4, -4, -4)));
	ASSERT(allLanesEqual(I32x4(-1, 2, -3, 4), I32x4(-1, 2, -3, 4)));
	ASSERT(!allLanesEqual(I32x4(-1, 2, 7, 4), I32x4(-1, 2, -3, 4)));

	// U32x4 Comparisons
	ASSERT(allLanesEqual(U32x4(4), U32x4(4, 4, 4, 4)));
	ASSERT(allLanesEqual(U32x4(1, 2, 3, 4), U32x4(1, 2, 3, 4)));
	ASSERT(!allLanesEqual(U32x4(1, 2, 7, 4), U32x4(1, 2, 3, 4)));

	// U16x8 Comparisons
	ASSERT(allLanesEqual(U16x8((uint16_t)8), U16x8(8, 8, 8, 8, 8, 8, 8, 8)));
	ASSERT(allLanesEqual(U16x8((uint32_t)8), U16x8(8, 0, 8, 0, 8, 0, 8, 0)));
	ASSERT(allLanesEqual(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(0, 2, 3, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(1, 0, 3, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(1, 2, 0, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(1, 2, 3, 0, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(1, 2, 3, 4, 0, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(1, 2, 3, 4, 5, 0, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(1, 2, 3, 4, 5, 6, 0, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(1, 2, 3, 4, 5, 6, 7, 0), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(1, 2, 0, 4, 5, 0, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(1, 0, 3, 4, 5, 6, 0, 0), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(0, 2, 3, 4, 0, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U16x8(0, 0, 0, 0, 0, 0, 0, 0), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));

	// U8x16 Comparisons
	ASSERT(allLanesEqual(U8x16((uint8_t)250), U8x16(250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250)));
	ASSERT(allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 0, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 0, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 0, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 0, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 0, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 0, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 0, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 0, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 0, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 0, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 0, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 0), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 251, 252, 6, 254, 255), U8x16(1, 2, 3, 4, 5, 9, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255)));
	ASSERT(!allLanesEqual(U8x16(1, 2, 3, 0, 5, 6, 7, 8, 9, 0, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 4, 8, 9, 10, 250, 251, 252, 253, 254, 255)));

	// Macros
	#ifdef USE_BASIC_SIMD
		{ // Truncate float to int
			SIMD_F32x4 f = LOAD_VECTOR_F32_SIMD(-1.01f, -0.99f, 0.99f, 1.01f);
			SIMD_I32x4 i = LOAD_VECTOR_I32_SIMD(-1, 0, 0, 1);
			ASSERT(allLanesEqual(I32x4(F32_TO_I32_SIMD(f)), I32x4(i)));
		}
		{ // Int to float
			SIMD_I32x4 n = LOAD_VECTOR_I32_SIMD(123   , 456   , 789   , -1000   );
			SIMD_F32x4 r = LOAD_VECTOR_F32_SIMD(123.0f, 456.0f, 789.0f, -1000.0f);
			ASSERT(allLanesEqual(F32x4(I32_TO_F32_SIMD(n)), F32x4(r)));
		}
		{ // Signed-unsigned cast
			ASSERT(allLanesEqual(I32x4(REINTERPRET_U32_TO_I32_SIMD(U32x4(1, 2, 3, 4).v)), I32x4(1, 2, 3, 4)));
			ASSERT(allLanesEqual(U32x4(REINTERPRET_I32_TO_U32_SIMD(I32x4(1, 2, 3, 4).v)), U32x4(1, 2, 3, 4)));
		}
		{ // F32x4
			SIMD_F32x4 a = LOAD_VECTOR_F32_SIMD(-1.3f, 2.5f, -3.4f, 4.7f);
			SIMD_F32x4 b = LOAD_VECTOR_F32_SIMD(5.2f, -2.0f, 0.1f, 1.9f);
			SIMD_F32x4 c = LOAD_SCALAR_F32_SIMD(0.5f);
			ASSERT(allLanesEqual(F32x4(ADD_F32_SIMD(a, b)), F32x4(-1.3f + 5.2f, 2.5f + -2.0f, -3.4f + 0.1f, 4.7f + 1.9f)));
			ASSERT(allLanesEqual(F32x4(SUB_F32_SIMD(a, b)), F32x4(-1.3f - 5.2f, 2.5f - -2.0f, -3.4f - 0.1f, 4.7f - 1.9f)));
			ASSERT(allLanesEqual(F32x4(ADD_F32_SIMD(a, c)), F32x4(-1.3f + 0.5f, 2.5f + 0.5f, -3.4f + 0.5f, 4.7f + 0.5f)));
			ASSERT(allLanesEqual(F32x4(SUB_F32_SIMD(a, c)), F32x4(-1.3f - 0.5f, 2.5f - 0.5f, -3.4f - 0.5f, 4.7f - 0.5f)));
			ASSERT(allLanesEqual(F32x4(MUL_F32_SIMD(a, c)), F32x4(-1.3f * 0.5f, 2.5f * 0.5f, -3.4f * 0.5f, 4.7f * 0.5f)));
			ASSERT(allLanesEqual(F32x4(MIN_F32_SIMD(a, b)), F32x4(-1.3f, -2.0f, -3.4f, 1.9f)));
			ASSERT(allLanesEqual(F32x4(MAX_F32_SIMD(a, b)), F32x4(5.2f, 2.5f, 0.1f, 4.7f)));
		}
		{ // I32x4
			SIMD_I32x4 a = LOAD_VECTOR_I32_SIMD(-1, 2, -3, 4);
			SIMD_I32x4 b = LOAD_VECTOR_I32_SIMD(5, -2, 0, 1);
			SIMD_I32x4 c = LOAD_SCALAR_I32_SIMD(4);
			ASSERT(allLanesEqual(I32x4(ADD_I32_SIMD(a, b)), I32x4(4, 0, -3, 5)));
			ASSERT(allLanesEqual(I32x4(SUB_I32_SIMD(a, b)), I32x4(-6, 4, -3, 3)));
			ASSERT(allLanesEqual(I32x4(ADD_I32_SIMD(a, c)), I32x4(3, 6, 1, 8)));
			ASSERT(allLanesEqual(I32x4(SUB_I32_SIMD(a, c)), I32x4(-5, -2, -7, 0)));
		}
		{ // U32x4
			SIMD_U32x4 a = LOAD_VECTOR_U32_SIMD(4, 5, 6, 7);
			SIMD_U32x4 b = LOAD_VECTOR_U32_SIMD(6, 5, 4, 3);
			SIMD_U32x4 c = LOAD_SCALAR_U32_SIMD(10);
			ASSERT(allLanesEqual(U32x4(ADD_U32_SIMD(a, b)), U32x4(c)));
			ASSERT(allLanesEqual(U32x4(ADD_U32_SIMD(a, c)), U32x4(14, 15, 16, 17)));
			ASSERT(allLanesEqual(U32x4(SUB_U32_SIMD(c, b)), U32x4(a)));
		}
		{ // U16x8
			SIMD_U16x8 a = LOAD_VECTOR_U16_SIMD(1, 2, 3, 4, 5, 6, 7, 8);
			SIMD_U16x8 b = LOAD_VECTOR_U16_SIMD(9, 8, 7, 6, 5, 4, 3, 2);
			SIMD_U16x8 c = LOAD_SCALAR_U16_SIMD(10);
			ASSERT(allLanesEqual(U16x8(ADD_U16_SIMD(a, b)), U16x8(c)));
			ASSERT(allLanesEqual(U16x8(ADD_U16_SIMD(a, c)), U16x8(11, 12, 13, 14, 15, 16, 17, 18)));
			ASSERT(allLanesEqual(U16x8(SUB_U16_SIMD(c, b)), U16x8(a)));
			ASSERT(allLanesEqual(U16x8(MUL_U16_SIMD(a, b)), U16x8(9, 16, 21, 24, 25, 24, 21, 16)));
		}
	#endif

	// Reinterpret (Depends on endianness!)
	ASSERT(allLanesEqual(U16x8(U32x4(12, 34, 56, 78)), U16x8(12, 0, 34, 0, 56, 0, 78, 0)));
	ASSERT(allLanesEqual(U16x8(12, 0, 34, 0, 56, 0, 78, 0).get_U32(), U32x4(12, 34, 56, 78)));

	// Reciprocal: 1 / x
	ASSERT(allLanesEqual(F32x4(0.5f, 1.0f, 2.0f, 4.0f).reciprocal(), F32x4(2.0f, 1.0f, 0.5f, 0.25f)));

	// Square root: sqrt(x)
	ASSERT(allLanesEqual(F32x4(1.0f, 4.0f, 9.0f, 100.0f).squareRoot(), F32x4(1.0f, 2.0f, 3.0f, 10.0f)));

	// Reciprocal square root: 1 / sqrt(x)
	ASSERT(allLanesEqual(F32x4(1.0f, 4.0f, 16.0f, 100.0f).reciprocalSquareRoot(), F32x4(1.0f, 0.5f, 0.25f, 0.1f)));

	// Minimum
	ASSERT(allLanesEqual(min(F32x4(1.1f, 2.2f, 3.3f, 4.4f), F32x4(5.0f, 3.0f, 1.0f, -1.0f)), F32x4(1.1f, 2.2f, 1.0f, -1.0f)));

	// Maximum
	ASSERT(allLanesEqual(max(F32x4(1.1f, 2.2f, 3.3f, 4.4f), F32x4(5.0f, 3.0f, 1.0f, -1.0f)), F32x4(5.0f, 3.0f, 3.3f, 4.4f)));

	// Clamp
	ASSERT(allLanesEqual(F32x4(-35.1f, 1.0f, 2.0f, 45.7f).clamp(-1.5f, 1.5f), F32x4(-1.5f, 1.0f, 1.5f, 1.5f)));

	// F32x4 operations
	ASSERT(allLanesEqual(F32x4(1.1f, -2.2f, 3.3f, 4.0f) + F32x4(2.2f, -4.4f, 6.6f, 8.0f), F32x4(3.3f, -6.6f, 9.9f, 12.0f)));
	ASSERT(allLanesEqual(F32x4(-1.5f, -0.5f, 0.5f, 1.5f) + 1.0f, F32x4(-0.5f, 0.5f, 1.5f, 2.5f)));
	ASSERT(allLanesEqual(1.0f + F32x4(-1.5f, -0.5f, 0.5f, 1.5f), F32x4(-0.5f, 0.5f, 1.5f, 2.5f)));
	ASSERT(allLanesEqual(F32x4(1.1f, 2.2f, 3.3f, 4.4f) - F32x4(0.1f, 0.2f, 0.3f, 0.4f), F32x4(1.0f, 2.0f, 3.0f, 4.0f)));
	ASSERT(allLanesEqual(F32x4(1.0f, 2.0f, 3.0f, 4.0f) - 0.5f, F32x4(0.5f, 1.5f, 2.5f, 3.5f)));
	ASSERT(allLanesEqual(0.5f - F32x4(1.0f, 2.0f, 3.0f, 4.0f), F32x4(-0.5f, -1.5f, -2.5f, -3.5f)));
	ASSERT(allLanesEqual(2.0f * F32x4(1.0f, 2.0f, 3.0f, 4.0f), F32x4(2.0f, 4.0f, 6.0f, 8.0f)));
	ASSERT(allLanesEqual(F32x4(1.0f, -2.0f, 3.0f, -4.0f) * -2.0f, F32x4(-2.0f, 4.0f, -6.0f, 8.0f)));
	ASSERT(allLanesEqual(F32x4(1.0f, -2.0f, 3.0f, -4.0f) * F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(1.0f, 4.0f, 9.0f, 16.0f)));
	ASSERT(allLanesEqual(-F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(-1.0f, 2.0f, -3.0f, 4.0f)));

	// I32x4 operations
	ASSERT(allLanesEqual(I32x4(1, 2, -3, 4) + I32x4(-2, 4, 6, 8), I32x4(-1, 6, 3, 12)));
	ASSERT(allLanesEqual(I32x4(1, -2, 3, 4) - 4, I32x4(-3, -6, -1, 0)));
	ASSERT(allLanesEqual(10 + I32x4(1, 2, 3, 4), I32x4(11, 12, 13, 14)));
	ASSERT(allLanesEqual(I32x4(1, 2, 3, 4) + I32x4(4), I32x4(5, 6, 7, 8)));
	ASSERT(allLanesEqual(I32x4(10) + I32x4(1, 2, 3, 4), I32x4(11, 12, 13, 14)));
	ASSERT(allLanesEqual(I32x4(-3, 6, -9, 12) * I32x4(1, 2, -3, -4), I32x4(-3, 12, 27, -48)));
	ASSERT(allLanesEqual(-I32x4(1, -2, 3, -4), I32x4(-1, 2, -3, 4)));

	// U32x4 operations
	ASSERT(allLanesEqual(U32x4(1, 2, 3, 4) + U32x4(2, 4, 6, 8), U32x4(3, 6, 9, 12)));
	ASSERT(allLanesEqual(U32x4(1, 2, 3, 4) + 4, U32x4(5, 6, 7, 8)));
	ASSERT(allLanesEqual(10 + U32x4(1, 2, 3, 4), U32x4(11, 12, 13, 14)));
	ASSERT(allLanesEqual(U32x4(1, 2, 3, 4) + U32x4(4), U32x4(5, 6, 7, 8)));
	ASSERT(allLanesEqual(U32x4(10) + U32x4(1, 2, 3, 4), U32x4(11, 12, 13, 14)));
	ASSERT(allLanesEqual(U32x4(3, 6, 9, 12) - U32x4(1, 2, 3, 4), U32x4(2, 4, 6, 8)));
	ASSERT(allLanesEqual(U32x4(3, 6, 9, 12) * U32x4(1, 2, 3, 4), U32x4(3, 12, 27, 48)));

	// U16x8 operations
	ASSERT(allLanesEqual(U16x8(1, 2, 3, 4, 5, 6, 7, 8) + U16x8(2, 4, 6, 8, 10, 12, 14, 16), U16x8(3, 6, 9, 12, 15, 18, 21, 24)));
	ASSERT(allLanesEqual(U16x8(1, 2, 3, 4, 5, 6, 7, 8) + 8, U16x8(9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(allLanesEqual(10 + U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(11, 12, 13, 14, 15, 16, 17, 18)));
	ASSERT(allLanesEqual(U16x8(1, 2, 3, 4, 5, 6, 7, 8) + U16x8((uint16_t)8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(allLanesEqual(U16x8((uint16_t)10) + U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(11, 12, 13, 14, 15, 16, 17, 18)));
	ASSERT(allLanesEqual(U16x8(3, 6, 9, 12, 15, 18, 21, 24) - U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(2, 4, 6, 8, 10, 12, 14, 16)));

	// U8x16 operations
	ASSERT(allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) + 2, U8x16(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18)));
	ASSERT(allLanesEqual(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) - 1, U8x16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)));
	ASSERT(allLanesEqual(
	  saturatedAddition(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 255), U8x16((uint8_t)250)),
	  U8x16(251, 252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255)
	));
	ASSERT(allLanesEqual(
	  saturatedSubtraction(
	  U8x16(128, 128, 128, 0, 255, 255,   0, 200, 123, 80, 46, 46, 46, 255, 255, 255),
	  U8x16(  0, 128, 255, 0, 255,   0, 255, 100,  23, 81, 45, 46, 47, 128, 127, 200)),
	  U8x16(128,   0,   0, 0,   0, 255,   0, 100, 100,  0,  1,  0,  0, 127, 128,  55)
	));

	// Saturated unsigned integer packing
	ASSERT(allLanesEqual(saturateToU8(U16x8(1, 2, 3, 4, 65535, 6, 7, 8), U16x8(9, 10, 11, 12, 1000, 14, 15, 16)), U8x16(1, 2, 3, 4, 255, 6, 7, 8, 9, 10, 11, 12, 255, 14, 15, 16)));

	// Unsigned integer unpacking
	ASSERT(allLanesEqual(lowerToU32(U16x8(1,2,3,4,5,6,7,8)), U32x4(1, 2, 3, 4)));
	ASSERT(allLanesEqual(higherToU32(U16x8(1,2,3,4,5,6,7,8)), U32x4(5, 6, 7, 8)));
	ASSERT(allLanesEqual(lowerToU16(U8x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)), U16x8(1,2,3,4,5,6,7,8)));
	ASSERT(allLanesEqual(higherToU16(U8x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)), U16x8(9,10,11,12,13,14,15,16)));

	// Reinterpretation
	ASSERT(allLanesEqual(
	  reinterpret_U8FromU32(U32x4(ENDIAN32_BYTE_0, ENDIAN32_BYTE_1, ENDIAN32_BYTE_2, ENDIAN32_BYTE_3)),
	  U8x16(
	    255, 0, 0, 0,
	    0, 255, 0, 0,
	    0, 0, 255, 0,
	    0, 0, 0, 255
	  )
	));
	ASSERT(allLanesEqual(
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
	));
	ASSERT(allLanesEqual(
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
	));

	// Bit mask
	ASSERT(allLanesEqual(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) & 0x0000FFFF, U32x4(0x0000FFFF, 0x00005678, 0x0000F0F0, 0x00000000)));
	ASSERT(allLanesEqual(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) & 0xFFFF0000, U32x4(0xFFFF0000, 0x12340000, 0xF0F00000, 0x00000000)));
	ASSERT(allLanesEqual(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) | 0x0000FFFF, U32x4(0xFFFFFFFF, 0x1234FFFF, 0xF0F0FFFF, 0x0000FFFF)));
	ASSERT(allLanesEqual(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) | 0xFFFF0000, U32x4(0xFFFFFFFF, 0xFFFF5678, 0xFFFFF0F0, 0xFFFF0000)));
	ASSERT(allLanesEqual(U32x4(0xFFFFFFFF, 0xFFF000FF, 0xF0F0F0F0, 0x12345678) & U32x4(0xFF00FF00, 0xFFFF0000, 0x000FF000, 0x0FF00FF0), U32x4(0xFF00FF00, 0xFFF00000, 0x0000F000, 0x02300670)));
	ASSERT(allLanesEqual(U32x4(0xF00F000F, 0xFFF000FF, 0x10010011, 0xABC00000) | U32x4(0x0000FF00, 0xFFFF0000, 0x000FF000, 0x000DEF00), U32x4(0xF00FFF0F, 0xFFFF00FF, 0x100FF011, 0xABCDEF00)));

	// Exclusive or
	ASSERT(allLanesEqual(U32x4(0xFFFFFFFF, 0x01234567, 0xF0F0F0F0, 0x00000000) ^ 0x0000FFFF, U32x4(0xFFFF0000, 0x0123BA98, 0xF0F00F0F, 0x0000FFFF)));

	// Bit shift with dynamic offset. (volatile to prevent constant propagation)
	volatile uint32_t offset = 1;
	ASSERT(allLanesEqual(U32x4(1, 2, 3, 4) << U32x4(offset), U32x4(2, 4, 6, 8)));
	offset = 2;
	ASSERT(allLanesEqual(U32x4(1, 2, 3, 4) << U32x4(offset), U32x4(4, 8, 12, 16)));
	offset = 3;
	ASSERT(allLanesEqual(U32x4(1, 2, 3, 4) << U32x4(offset), U32x4(8, 16, 24, 32)));
	offset = 4;
	ASSERT(allLanesEqual(U32x4(1, 2, 3, 4) << U32x4(offset), U32x4(16, 32, 48, 64)));
	offset = 1;
	ASSERT(allLanesEqual(U32x4(1, 2, 3, 4) >> U32x4(offset), U32x4(0, 1, 1, 2)));
	ASSERT(allLanesEqual(U32x4(2, 4, 6, 8) >> U32x4(offset), U32x4(1, 2, 3, 4)));
	offset = 2;
	ASSERT(allLanesEqual(U32x4(2, 4, 6, 8) >> U32x4(offset), U32x4(0, 1, 1, 2)));
	
	ASSERT(allLanesEqual(bitShiftLeftImmediate<1>(U32x4(1, 2, 3, 4)), U32x4(2, 4, 6, 8)));
	ASSERT(allLanesEqual(bitShiftLeftImmediate<2>(U32x4(1, 2, 3, 4)), U32x4(4, 8, 12, 16)));
	ASSERT(allLanesEqual(bitShiftLeftImmediate<3>(U32x4(1, 2, 3, 4)), U32x4(8, 16, 24, 32)));
	ASSERT(allLanesEqual(bitShiftLeftImmediate<4>(U32x4(1, 2, 3, 4)), U32x4(16, 32, 48, 64)));
	ASSERT(allLanesEqual(bitShiftRightImmediate<1>(U32x4(1, 2, 3, 4)), U32x4(0, 1, 1, 2)));
	ASSERT(allLanesEqual(bitShiftRightImmediate<1>(U32x4(2, 4, 6, 8)), U32x4(1, 2, 3, 4)));
	ASSERT(allLanesEqual(bitShiftRightImmediate<2>(U32x4(2, 4, 6, 8)), U32x4(0, 1, 1, 2)));
	ASSERT(allLanesEqual(bitShiftLeftImmediate<4>(U32x4(0x0AB12CD0, 0xFFFFFFFF, 0x12345678, 0xF0000000)), U32x4(0xAB12CD00, 0xFFFFFFF0, 0x23456780, 0x00000000)));
	ASSERT(allLanesEqual(bitShiftRightImmediate<4>(U32x4(0x0AB12CD0, 0xFFFFFFFF, 0x12345678, 0x0000000F)), U32x4(0x00AB12CD, 0x0FFFFFFF, 0x01234567, 0x00000000)));

	// Element shift with insert
	ASSERT(allLanesEqual(vectorExtract_0(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(1, 2, 3, 4)));
	ASSERT(allLanesEqual(vectorExtract_1(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(2, 3, 4, 5)));
	ASSERT(allLanesEqual(vectorExtract_2(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(3, 4, 5, 6)));
	ASSERT(allLanesEqual(vectorExtract_3(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(4, 5, 6, 7)));
	ASSERT(allLanesEqual(vectorExtract_4(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(5, 6, 7, 8)));
	ASSERT(allLanesEqual(vectorExtract_0(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(123, 4294967295, 712, 45)));
	ASSERT(allLanesEqual(vectorExtract_1(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(4294967295, 712, 45, 850514)));
	ASSERT(allLanesEqual(vectorExtract_2(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(712, 45, 850514, 27)));
	ASSERT(allLanesEqual(vectorExtract_3(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(45, 850514, 27, 0)));
	ASSERT(allLanesEqual(vectorExtract_4(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(850514, 27, 0, 174)));
	ASSERT(allLanesEqual(vectorExtract_0(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(1, 2, 3, 4)));
	ASSERT(allLanesEqual(vectorExtract_1(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(2, 3, 4, 5)));
	ASSERT(allLanesEqual(vectorExtract_2(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(3, 4, 5, 6)));
	ASSERT(allLanesEqual(vectorExtract_3(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(4, 5, 6, 7)));
	ASSERT(allLanesEqual(vectorExtract_4(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(5, 6, 7, 8)));
	ASSERT(allLanesEqual(vectorExtract_0(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(123, 8462784, -712, 45)));
	ASSERT(allLanesEqual(vectorExtract_1(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(8462784, -712, 45, -37562)));
	ASSERT(allLanesEqual(vectorExtract_2(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(-712, 45, -37562, 27)));
	ASSERT(allLanesEqual(vectorExtract_3(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(45, -37562, 27, 0)));
	ASSERT(allLanesEqual(vectorExtract_4(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(-37562, 27, 0, 174)));
	ASSERT(allLanesEqual(vectorExtract_0(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(1.0f, -2.0f, 3.0f, -4.0f)));
	ASSERT(allLanesEqual(vectorExtract_1(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(-2.0f, 3.0f, -4.0f, 5.0f)));
	ASSERT(allLanesEqual(vectorExtract_2(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(3.0f, -4.0f, 5.0f, 6.0f)));
	ASSERT(allLanesEqual(vectorExtract_3(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(-4.0f, 5.0f, 6.0f, -7.0f)));
	ASSERT(allLanesEqual(vectorExtract_4(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(5.0f, 6.0f, -7.0f, 8.0f)));
	ASSERT(allLanesEqual(vectorExtract_0(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(allLanesEqual(vectorExtract_1(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(2, 3, 4, 5, 6, 7, 8, 9)));
	ASSERT(allLanesEqual(vectorExtract_2(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(3, 4, 5, 6, 7, 8, 9, 10)));
	ASSERT(allLanesEqual(vectorExtract_3(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(4, 5, 6, 7, 8, 9, 10, 11)));
	ASSERT(allLanesEqual(vectorExtract_4(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(5, 6, 7, 8, 9, 10, 11, 12)));
	ASSERT(allLanesEqual(vectorExtract_5(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(6, 7, 8, 9, 10, 11, 12, 13)));
	ASSERT(allLanesEqual(vectorExtract_6(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(7, 8, 9, 10, 11, 12, 13, 14)));
	ASSERT(allLanesEqual(vectorExtract_7(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(8, 9, 10, 11, 12, 13, 14, 15)));
	ASSERT(allLanesEqual(vectorExtract_8(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(allLanesEqual(vectorExtract_0(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(allLanesEqual(vectorExtract_1(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17)));
	ASSERT(allLanesEqual(vectorExtract_2(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18)));
	ASSERT(allLanesEqual(vectorExtract_3(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19)));
	ASSERT(allLanesEqual(vectorExtract_4(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20)));
	ASSERT(allLanesEqual(vectorExtract_5(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21)));
	ASSERT(allLanesEqual(vectorExtract_6(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22)));
	ASSERT(allLanesEqual(vectorExtract_7(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23)));
	ASSERT(allLanesEqual(vectorExtract_8(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24)));
	ASSERT(allLanesEqual(vectorExtract_9(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25)));
	ASSERT(allLanesEqual(vectorExtract_10(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26)));
	ASSERT(allLanesEqual(vectorExtract_11(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27)));
	ASSERT(allLanesEqual(vectorExtract_12(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28)));
	ASSERT(allLanesEqual(vectorExtract_13(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29)));
	ASSERT(allLanesEqual(vectorExtract_14(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30)));
	ASSERT(allLanesEqual(vectorExtract_15(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31)));
	ASSERT(allLanesEqual(vectorExtract_16(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)));

	// TODO: Move SIMD extra into simd.h and implement emulation.
	#ifdef USE_SIMD_EXTRA
		SIMD_U32x4 a = U32x4(1, 2, 3, 4).v;
		SIMD_U32x4 b = U32x4(5, 6, 7, 8).v;
		SIMD_U32x4x2 c = ZIP_U32_SIMD(a, b);
		ASSERT(allLanesEqual(U32x4(c.val[0]), U32x4(1, 5, 2, 6)));
		ASSERT(allLanesEqual(U32x4(c.val[1]), U32x4(3, 7, 4, 8)));
		SIMD_U32x4 d = ZIP_LOW_U32_SIMD(a, b);
		SIMD_U32x4 e = ZIP_HIGH_U32_SIMD(a, b);
		ASSERT(allLanesEqual(U32x4(d), U32x4(1, 5, 2, 6)));
		ASSERT(allLanesEqual(U32x4(e), U32x4(3, 7, 4, 8)));
	#endif

	// 256-bit SIMD tests (emulated using scalar operations if the test is not compiled with AVX2 enabled)

	// F32x8 Comparisons
	ASSERT(allLanesEqual(F32x8(1.5f), F32x8(1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f, 1.5f)));
	ASSERT(allLanesEqual(F32x8(-1.5f), F32x8(-1.5f, -1.5f, -1.5f, -1.5f, -1.5f, -1.5f, -1.5f, -1.5f)));
	ASSERT(allLanesEqual(F32x8(1.2f, 3.4f, 5.6f, 7.8f, -2.4f, 452.351f, 1000000.0f, -1000.0f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, -2.4f, 452.351f, 1000000.0f, -1000.0f)));
	ASSERT(!allLanesEqual(F32x8(1.3f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f)));
	ASSERT(!allLanesEqual(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, -1.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f)));
	ASSERT(!allLanesEqual(F32x8(1.2f, 3.4f, 5.5f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f)));
	ASSERT(!allLanesEqual(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, -7.8f, 5.3f, 6.7f, 1.4f, -5.2f)));
	ASSERT(!allLanesEqual(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 0.0f, 6.7f, 1.4f, -5.2f)));
	ASSERT(!allLanesEqual(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.69f, 1.4f, -5.2f)));
	ASSERT(!allLanesEqual(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.3f, -5.2f)));
	ASSERT(!allLanesEqual(F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, -5.2f), F32x8(1.2f, 3.4f, 5.6f, 7.8f, 5.3f, 6.7f, 1.4f, 5.2f)));

	// I32x8 Comparisons
	ASSERT(allLanesEqual(I32x8(4), I32x8(4, 4, 4, 4, 4, 4, 4, 4)));
	ASSERT(allLanesEqual(I32x8(-4), I32x8(-4, -4, -4, -4, -4, -4, -4, -4)));
	ASSERT(allLanesEqual(I32x8(-1, 2, -3, 4, -5, 6, -7, 8), I32x8(-1, 2, -3, 4, -5, 6, -7, 8)));
	ASSERT(!allLanesEqual(I32x8(-1, 2, 7, 4, 8, 3, 5, 45), I32x8(-1, 2, -3, 4, 8, 3, 5, 45)));

	// U32x8 Comparisons
	ASSERT(allLanesEqual(U32x8(4), U32x8(4, 4, 4, 4, 4, 4, 4, 4)));
	ASSERT(allLanesEqual(U32x8(1, 2, 3, 4, 5, 6, 7, 8), U32x8(1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(!allLanesEqual(U32x8(1, 2, 3, 4, 5, 6, 12, 8), U32x8(1, 2, 3, 4, 5, 6, 7, 8)));

	// U16x16 Comparisons
	ASSERT(allLanesEqual(U16x16((uint16_t)8), U16x16(8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8)));
	ASSERT(allLanesEqual(U16x16((uint32_t)8), U16x16(8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0, 8, 0)));
	ASSERT(allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 0, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 0, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 0, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 5, 0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 0, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 0, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9,  0, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,  0, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,  0, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,  0, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,  0, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,  0, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,  0), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 2, 0, 4, 5, 0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(1, 0, 3, 4, 5, 6, 0, 0, 9, 10, 11, 12, 13,  0, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(0, 2, 3, 4, 0, 6, 7, 8, 9, 10, 11, 0,  13, 14, 15, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
	ASSERT(!allLanesEqual(U16x16(0, 0, 0, 0, 0, 0, 0, 0, 9, 10, 11, 0,  13, 14,  0, 16), U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));

	// U8x32 Comparisons
	ASSERT(allLanesEqual(U8x32((uint8_t)250), U8x32(250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250)));
	ASSERT(!allLanesEqual(U8x32((uint8_t)250), U8x32(250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 100, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250)));
	ASSERT(!allLanesEqual(U8x32((uint8_t)250), U8x32(0, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250)));
	ASSERT(!allLanesEqual(U8x32((uint8_t)250), U8x32(250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 0)));

	// Reinterpret (Depends on endianness!)
	ASSERT(allLanesEqual(U16x16(U32x8(12, 34, 56, 78, 11, 22, 33, 44)), U16x16(12, 0, 34, 0, 56, 0, 78, 0, 11, 0, 22, 0, 33, 0, 44, 0)));
	ASSERT(allLanesEqual(U16x16(U32x8(12, 34, 56, 78, 11, 22, 33, 131116)), U16x16(12, 0, 34, 0, 56, 0, 78, 0, 11, 0, 22, 0, 33, 0, 44, 2)));
	ASSERT(allLanesEqual(U16x16(12, 0, 34, 0, 56, 0, 78, 0, 11, 0, 22, 0, 33, 0, 44, 2).get_U32(), U32x8(12, 34, 56, 78, 11, 22, 33, 131116)));

	// Reciprocal: 1 / x
	ASSERT(allLanesEqual(F32x8(0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 10.0f, 100.0f, 1000.0f).reciprocal(), F32x8(2.0f, 1.0f, 0.5f, 0.25f, 0.125f, 0.1f, 0.01f, 0.001f)));

	// Square root: sqrt(x)
	ASSERT(allLanesEqual(F32x8(1.0f, 4.0f, 9.0f, 100.0f, 64.0f, 256.0f, 1024.0f, 4096.0f).squareRoot(), F32x8(1.0f, 2.0f, 3.0f, 10.0f, 8.0f, 16.0f, 32.0f, 64.0f)));

	// Reciprocal square root: 1 / sqrt(x)
	ASSERT(allLanesEqual(F32x8(1.0f, 4.0f, 16.0f, 100.0f, 400.0f, 64.0f, 25.0f, 100.0f).reciprocalSquareRoot(), F32x8(1.0f, 0.5f, 0.25f, 0.1f, 0.05f, 0.125f, 0.2f, 0.1f)));

	// Minimum
	ASSERT(allLanesEqual(min(F32x8(1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 8.8f), F32x8(5.0f, 3.0f, 1.0f, -1.0f, 4.0f, 5.0f, -2.5f, 10.0f)), F32x8(1.1f, 2.2f, 1.0f, -1.0f, 4.0f, 5.0f, -2.5f, 8.8f)));

	// Maximum
	ASSERT(allLanesEqual(max(F32x8(1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 8.8f), F32x8(5.0f, 3.0f, 1.0f, -1.0f, 4.0f, 5.0f, -2.5f, 10.0f)), F32x8(5.0f, 3.0f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 10.0f)));

	// Clamp
	ASSERT(allLanesEqual(F32x8(-35.1f, 1.0f, 2.0f, 45.7f, 0.0f, -1.0f, 2.1f, -1.9f).clamp(-1.5f, 1.5f), F32x8(-1.5f, 1.0f, 1.5f, 1.5f, 0.0f, -1.0f, 1.5f, -1.5f)));

	// F32x8 operations
	ASSERT(allLanesEqual(F32x8(1.1f, -2.2f, 3.3f, 4.0f, 1.4f, 2.3f, 3.2f, 4.1f) + F32x8(2.2f, -4.4f, 6.6f, 8.0f, 4.11f, 3.22f, 2.33f, 1.44f), F32x8(3.3f, -6.6f, 9.9f, 12.0f, 5.51f, 5.52f, 5.53f, 5.54f)));
	ASSERT(allLanesEqual(F32x8(-1.5f, -0.5f, 0.5f, 1.5f, 1000.0f, 2000.0f, -4000.0f, -1500.0f) + 1.0f, F32x8(-0.5f, 0.5f, 1.5f, 2.5f, 1001.0f, 2001.0f, -3999.0f, -1499.0f)));
	ASSERT(allLanesEqual(1.0f + F32x8(-1.5f, -0.5f, 0.5f, 1.5f, 1000.0f, 2000.0f, -4000.0f, -1500.0f), F32x8(-0.5f, 0.5f, 1.5f, 2.5f, 1001.0f, 2001.0f, -3999.0f, -1499.0f)));
	ASSERT(allLanesEqual(F32x8(1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 8.8f) - F32x8(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f), F32x8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f)));
	ASSERT(allLanesEqual(F32x8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f) - 0.5f, F32x8(0.5f, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f)));
	ASSERT(allLanesEqual(0.5f - F32x8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f), F32x8(-0.5f, -1.5f, -2.5f, -3.5f, -4.5f, -5.5f, -6.5f, -7.5f)));
	ASSERT(allLanesEqual(2.0f * F32x8(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f), F32x8(2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f)));
	ASSERT(allLanesEqual(F32x8(1.0f, -2.0f, 3.0f, -4.0f, 5.0f, -6.0f, 7.0f, -8.0f) * -2.0f, F32x8(-2.0f, 4.0f, -6.0f, 8.0f, -10.0f, 12.0f, -14.0f, 16.0f)));
	ASSERT(allLanesEqual(F32x8(1.0f, -2.0f, 3.0f, -4.0f, 5.0f, -6.0f, 7.0f, -8.0f) * F32x8(1.0f, -2.0f, 3.0f, -4.0f, 5.0f, -6.0f, 7.0f, -8.0f), F32x8(1.0f, 4.0f, 9.0f, 16.0f, 25.0f, 36.0f, 49.0f, 64.0f)));
	ASSERT(allLanesEqual(-F32x8(1.0f, -2.0f, 3.0f, -4.0f, 5.0f, -6.0f, 7.0f, -8.0f), F32x8(-1.0f, 2.0f, -3.0f, 4.0f, -5.0f, 6.0f, -7.0f, 8.0f)));

	// I32x8 operations
	ASSERT(allLanesEqual(I32x8(1, 2, 3, 4, 5, 6, 7, 8) - 1, I32x8(0, 1, 2, 3, 4, 5, 6, 7)));
	ASSERT(allLanesEqual(1 - I32x8(1, 2, 3, 4, 5, 6, 7, 8), I32x8(0, -1, -2, -3, -4, -5, -6, -7)));
	ASSERT(allLanesEqual(2 * I32x8(1, 2, 3, 4, 5, 6, 7, 8), I32x8(2, 4, 6, 8, 10, 12, 14, 16)));
	ASSERT(allLanesEqual(I32x8(1, -2, 3, -4, 5, -6, 7, -8) * -2, I32x8(-2, 4, -6, 8, -10, 12, -14, 16)));
	ASSERT(allLanesEqual(I32x8(1, -2, 3, -4, 5, -6, 7, -8) * I32x8(1, -2, 3, -4, 5, -6, 7, -8), I32x8(1, 4, 9, 16, 25, 36, 49, 64)));
	ASSERT(allLanesEqual(-I32x8(1, -2, 3, -4, 5, -6, 7, -8), I32x8(-1, 2, -3, 4, -5, 6, -7, 8)));

	// U32x8 operations
	ASSERT(allLanesEqual(U32x8(1, 2, 3, 4, 5, 6, 7, 8) - 1, U32x8(0, 1, 2, 3, 4, 5, 6, 7)));
	ASSERT(allLanesEqual(10 - U32x8(1, 2, 3, 4, 5, 6, 7, 8), U32x8(9, 8, 7, 6, 5, 4, 3, 2)));
	ASSERT(allLanesEqual(2 * U32x8(1, 2, 3, 4, 5, 6, 7, 8), U32x8(2, 4, 6, 8, 10, 12, 14, 16)));
	ASSERT(allLanesEqual(U32x8(1, 2, 3, 4, 5, 6, 7, 8) * 2, U32x8(2, 4, 6, 8, 10, 12, 14, 16)));
	ASSERT(allLanesEqual(U32x8(1, 2, 3, 4, 5, 6, 7, 8) * U32x8(1, 2, 3, 4, 5, 6, 7, 8), U32x8(1, 4, 9, 16, 25, 36, 49, 64)));

	// U16x16 operations
	ASSERT(allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) + U16x16(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32), U16x16(3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48)));
	ASSERT(allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) + 8, U16x16(9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24)));
	ASSERT(allLanesEqual(8 + U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24)));
	ASSERT(allLanesEqual(U16x16(3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48) - U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32)));
	ASSERT(allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) - 1, U16x16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)));
	ASSERT(allLanesEqual(16 - U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)));
	ASSERT(allLanesEqual(U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) * 2, U16x16(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32)));
	ASSERT(allLanesEqual(2 * U16x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U16x16(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32)));

	// U8x32 operations
	ASSERT(allLanesEqual(
	      U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)
	    + U8x32( 2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64),
	      U8x32( 3,  6,  9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93, 96)));
	ASSERT(allLanesEqual(
	      U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32) + 5,
	      U8x32( 6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37)));
	ASSERT(allLanesEqual(
	  5 + U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32),
	      U8x32( 6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37)));
	ASSERT(allLanesEqual(
	      U8x32( 3,  6,  9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93, 96)
	    - U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32),
	      U8x32( 2,  4,  6,  8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64)));
	ASSERT(allLanesEqual(
	      U8x32( 6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37) - 5,
	      U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)));
	ASSERT(allLanesEqual(
	 33 - U8x32( 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32),
	      U8x32(32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1)));
	ASSERT(allLanesEqual(
	  saturatedAddition(
	    U8x32(  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,255),
	    U8x32((uint8_t)240)),
	    U8x32(241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255)
	));
	ASSERT(allLanesEqual(
	  saturatedSubtraction(
	    U8x32(  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,255),
	    U8x32((uint8_t)16)),
	    U8x32(  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,239)
	));

	// Saturated unsigned integer packing
	ASSERT(allLanesEqual(saturateToU8(
	  U16x16(1, 2, 3, 4, 65535, 6, 7, 8, 9, 10, 11, 12, 1000, 14, 15, 16), U16x16(17, 18, 19, 20, 21, 22, 23, 65535, 25, 26, 27, 28, 29, 30, 31, 32)),
	  U8x32( 1, 2, 3, 4, 255,   6, 7, 8, 9, 10, 11, 12,  255, 14, 15, 16,         17, 18, 19, 20, 21, 22, 23,   255, 25, 26, 27, 28, 29, 30, 31, 32)));

	// Unsigned integer unpacking
	ASSERT(allLanesEqual(lowerToU32(U16x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)), U32x8(1,2,3,4,5,6,7,8)));
	ASSERT(allLanesEqual(higherToU32(U16x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)), U32x8(9,10,11,12,13,14,15,16)));
	ASSERT(allLanesEqual(lowerToU32(U16x16(1,2,3,4,5,6,65535,8,9,10,11,12,13,1000,15,16)), U32x8(1,2,3,4,5,6,65535,8)));
	ASSERT(allLanesEqual(higherToU32(U16x16(1,2,3,4,5,6,65535,8,9,10,11,12,13,1000,15,16)), U32x8(9,10,11,12,13,1000,15,16)));
	ASSERT(allLanesEqual(lowerToU16(U8x32(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,255,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,255)), U16x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,255)));
	ASSERT(allLanesEqual(higherToU16(U8x32(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,255,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,255)), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,255)));

	// Bitwise operations
	ASSERT(allLanesEqual(
	    U32x8(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  & 0x0000FFFF,
	    U32x8(0x0000FFFF, 0x00005678, 0x0000F0F0, 0x00000000, 0x0000EEEE, 0x00004321, 0x00000F0F, 0x00000001)));
	ASSERT(allLanesEqual(
	    U32x8(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  & 0xFFFF0000,
	    U32x8(0xFFFF0000, 0x12340000, 0xF0F00000, 0x00000000, 0xEEEE0000, 0x87650000, 0x0F0F0000, 0x00010000)));
	ASSERT(allLanesEqual(
	    U32x8(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  | 0x0000FFFF,
	    U32x8(0xFFFFFFFF, 0x1234FFFF, 0xF0F0FFFF, 0x0000FFFF, 0xEEEEFFFF, 0x8765FFFF, 0x0F0FFFFF, 0x0001FFFF)));
	ASSERT(allLanesEqual(
	    U32x8(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  | 0xFFFF0000,
	    U32x8(0xFFFFFFFF, 0xFFFF5678, 0xFFFFF0F0, 0xFFFF0000, 0xFFFFEEEE, 0xFFFF4321, 0xFFFF0F0F, 0xFFFF0001)));
	ASSERT(allLanesEqual(
	    U32x8(0xFFFFFFFF, 0xFFF000FF, 0xF0F0F0F0, 0x12345678, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  & U32x8(0xFF00FF00, 0xFFFF0000, 0x000FF000, 0x0FF00FF0, 0xF00FF00F, 0x00FFFF00, 0xF0F0F0F0, 0x0000FFFF),
	    U32x8(0xFF00FF00, 0xFFF00000, 0x0000F000, 0x02300670, 0xE00EE00E, 0x00654300, 0x00000000, 0x00000001)));
	ASSERT(allLanesEqual(
	    U32x8(0xFFFFFFFF, 0xFFF000FF, 0xF0F0F0F0, 0x12345678, 0xEEEEEEEE, 0x87654321, 0x0F0F0F0F, 0x00010001)
	  | U32x8(0xFF00FF00, 0xFFFF0000, 0x000FF000, 0x0FF00FF0, 0xF00FF00F, 0x00FFFF00, 0xF0F0F0F0, 0x0000FFFF),
	    U32x8(0xFFFFFFFF, 0xFFFF00FF, 0xF0FFF0F0, 0x1FF45FF8, 0xFEEFFEEF, 0x87FFFF21, 0xFFFFFFFF, 0x0001FFFF)));
	ASSERT(allLanesEqual(
	    U32x8(0b11001100110000110101010010110011, 0b00101011001011101010001101111001, 0b11001010000110111010010100101100, 0b01010111010001010010101110010110, 0b10101110100110100010101011011001, 0b00101110100111010001101010110000, 0b11101010001011100010101110001111, 0b00101010111100010110010110001000)
	  ^ U32x8(0b00101101001110100011010010100001, 0b10101110100101000011101001010011, 0b00101011100101001011000010100100, 0b11010011101001000110010110110111, 0b00111100101000101010001101001010, 0b00101110100110000111110011010101, 0b11001010010101010010110010101000, 0b11110000111100001111000011110000),
	    U32x8(0b11100001111110010110000000010010, 0b10000101101110101001100100101010, 0b11100001100011110001010110001000, 0b10000100111000010100111000100001, 0b10010010001110001000100110010011, 0b00000000000001010110011001100101, 0b00100000011110110000011100100111, 0b11011010000000011001010101111000)));

	// Bit shift with immediate scalar
	ASSERT(allLanesEqual(bitShiftLeftImmediate <1>(U32x8(1,  2,  3,  4,  5,  6,  7,  8)), U32x8( 2,  4,  6,  8, 10, 12, 14, 16)));
	ASSERT(allLanesEqual(bitShiftLeftImmediate <2>(U32x8(1,  2,  3,  4,  5,  6,  7,  8)), U32x8( 4,  8, 12, 16, 20, 24, 28, 32)));
	ASSERT(allLanesEqual(bitShiftLeftImmediate <3>(U32x8(1,  2,  3,  4,  5,  6,  7,  8)), U32x8( 8, 16, 24, 32, 40, 48, 56, 64)));
	ASSERT(allLanesEqual(bitShiftLeftImmediate <4>(U32x8(1,  2,  3,  4,  5,  6,  7,  8)), U32x8(16, 32, 48, 64, 80, 96,112,128)));
	ASSERT(allLanesEqual(bitShiftRightImmediate<1>(U32x8(1,  2,  3,  4,  5,  6,  7,  8)), U32x8( 0,  1,  1,  2,  2,  3,  3,  4)));
	ASSERT(allLanesEqual(bitShiftRightImmediate<1>(U32x8(2,  4,  6,  8, 10, 12, 14, 16)), U32x8( 1,  2,  3,  4,  5,  6,  7,  8)));
	ASSERT(allLanesEqual(bitShiftRightImmediate<2>(U32x8(2,  4,  6,  8, 10, 12, 14, 16)), U32x8( 0,  1,  1,  2,  2,  3,  3,  4)));

	// Bit shift with variable offsets
	ASSERT(allLanesEqual(U32x4(1u, 2u, 3u, 4u) << U32x4(2u, 4u, 3u, 1u), U32x4(4u, 32u, 24u, 8u)));
	ASSERT(allLanesEqual(U32x4(64u, 32u, 5u, 8u) >> U32x4(2u, 1u, 2u, 0u), U32x4(16u, 16u, 1u, 8u)));
	ASSERT(allLanesEqual(U32x8(1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u) << U32x8(2u, 4u, 3u, 1u, 0u, 1u, 2u, 1u), U32x8(4u, 32u, 24u, 8u, 5u, 12u, 28u, 16u)));
	ASSERT(allLanesEqual(U32x8(64u, 32u, 5u, 8u, 128u, 64u, 128u, 256u) >> U32x8(2u, 4u, 3u, 1u, 3u, 1u, 2u, 1u), U32x8(16u, 2u, 0u, 4u, 16u, 32u, 32u, 128u)));

	ASSERT(allLanesEqual(
	    bitShiftLeftImmediate<4>(U32x8(0x0AB12CD0, 0xFFFFFFFF, 0x12345678, 0xF0000000, 0x87654321, 0x48484848, 0x76437643, 0x11111111)),
	                             U32x8(0xAB12CD00, 0xFFFFFFF0, 0x23456780, 0x00000000, 0x76543210, 0x84848480, 0x64376430, 0x11111110)));
	ASSERT(allLanesEqual(
	    bitShiftRightImmediate<4>(U32x8(0x0AB12CD0, 0xFFFFFFFF, 0x12345678, 0x0000000F, 0x87654321, 0x48484848, 0x76437643, 0x11111111)),
	                              U32x8(0x00AB12CD, 0x0FFFFFFF, 0x01234567, 0x00000000, 0x08765432, 0x04848484, 0x07643764, 0x01111111)));

	// Element shift with insert
	ASSERT(allLanesEqual(vectorExtract_0(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                     U32x8( 1, 2, 3, 4, 5, 6, 7, 8)));
	ASSERT(allLanesEqual(vectorExtract_1(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                        U32x8( 2, 3, 4, 5, 6, 7, 8,         9)));
	ASSERT(allLanesEqual(vectorExtract_2(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                           U32x8( 3, 4, 5, 6, 7, 8,         9,10)));
	ASSERT(allLanesEqual(vectorExtract_3(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                              U32x8( 4, 5, 6, 7, 8,         9,10,11)));
	ASSERT(allLanesEqual(vectorExtract_4(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                                 U32x8( 5, 6, 7, 8,         9,10,11,12)));
	ASSERT(allLanesEqual(vectorExtract_5(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                                    U32x8( 6, 7, 8,         9,10,11,12,13)));
	ASSERT(allLanesEqual(vectorExtract_6(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                                       U32x8( 7, 8,         9,10,11,12,13,14)));
	ASSERT(allLanesEqual(vectorExtract_7(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                                          U32x8( 8,         9,10,11,12,13,14,15)));
	ASSERT(allLanesEqual(vectorExtract_8(U32x8( 1, 2, 3, 4, 5, 6, 7, 8), U32x8( 9,10,11,12,13,14,15,16)),
	                                                                     U32x8( 9,10,11,12,13,14,15,16)));
	ASSERT(allLanesEqual(vectorExtract_5(U32x8( 1, 2, 3, 4, 5, 6, 7, 4294967295), U32x8( 9,10,11,1000,13,14,15,16)),
	                                                    U32x8( 6, 7, 4294967295,         9,10,11,1000,13)));
	ASSERT(allLanesEqual(vectorExtract_0(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                     I32x8( 1,-2, 3, 4,-5, 6, 7, 8)));
	ASSERT(allLanesEqual(vectorExtract_1(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                        I32x8(-2, 3, 4,-5, 6, 7, 8,         9)));
	ASSERT(allLanesEqual(vectorExtract_2(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                           I32x8( 3, 4,-5, 6, 7, 8,         9,10)));
	ASSERT(allLanesEqual(vectorExtract_3(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                              I32x8( 4,-5, 6, 7, 8,         9,10,11)));
	ASSERT(allLanesEqual(vectorExtract_4(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                                 I32x8(-5, 6, 7, 8,         9,10,11,-12)));
	ASSERT(allLanesEqual(vectorExtract_5(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                                    I32x8( 6, 7, 8,         9,10,11,-12,13)));
	ASSERT(allLanesEqual(vectorExtract_6(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                                       I32x8( 7, 8,         9,10,11,-12,13,14)));
	ASSERT(allLanesEqual(vectorExtract_7(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                                          I32x8( 8,         9,10,11,-12,13,14,15)));
	ASSERT(allLanesEqual(vectorExtract_8(I32x8( 1,-2, 3, 4,-5, 6, 7, 8), I32x8( 9,10,11,-12,13,14,15,-16)),
	                                                                     I32x8( 9,10,11,-12,13,14,15,-16)));
	ASSERT(allLanesEqual(vectorExtract_0(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                     F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f)));
	ASSERT(allLanesEqual(vectorExtract_1(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                          F32x8( -2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f,         9.0f)));
	ASSERT(allLanesEqual(vectorExtract_2(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                 F32x8( 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f,         9.0f, 10.0f)));
	ASSERT(allLanesEqual(vectorExtract_3(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                       F32x8( 4.0f,-5.0f, 6.0f, 7.0f, 8.0f,         9.0f, 10.0f, 11.0f)));
	ASSERT(allLanesEqual(vectorExtract_4(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                             F32x8(-5.0f, 6.0f, 7.0f, 8.0f,         9.0f, 10.0f, 11.0f,-12.0f)));
	ASSERT(allLanesEqual(vectorExtract_5(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                                   F32x8( 6.0f, 7.0f, 8.0f,         9.0f, 10.0f, 11.0f,-12.0f, 13.0f)));
	ASSERT(allLanesEqual(vectorExtract_6(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                                         F32x8( 7.0f, 8.0f,         9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f)));
	ASSERT(allLanesEqual(vectorExtract_7(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                                               F32x8( 8.0f,         9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f)));
	ASSERT(allLanesEqual(vectorExtract_8(F32x8( 1.1f,-2.2f, 3.0f, 4.0f,-5.0f, 6.0f, 7.0f, 8.0f), F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)),
	                                                                                             F32x8( 9.0f, 10.0f, 11.0f,-12.0f, 13.0f, 14.0f, 15.0f,-16.0f)));
	ASSERT(allLanesEqual(vectorExtract_0 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                      U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16)));
	ASSERT(allLanesEqual(vectorExtract_1 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                         U16x16( 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,         17)));
	ASSERT(allLanesEqual(vectorExtract_2 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                            U16x16( 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,         17,18)));
	ASSERT(allLanesEqual(vectorExtract_3 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                               U16x16( 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,         17,18,19)));
	ASSERT(allLanesEqual(vectorExtract_4 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                  U16x16( 5, 6, 7, 8, 9,10,11,12,13,14,15,16,         17,18,19,20)));
	ASSERT(allLanesEqual(vectorExtract_5 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                     U16x16( 6, 7, 8, 9,10,11,12,13,14,15,16,         17,18,19,20,21)));
	ASSERT(allLanesEqual(vectorExtract_6 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                        U16x16( 7, 8, 9,10,11,12,13,14,15,16,         17,18,19,20,21,22)));
	ASSERT(allLanesEqual(vectorExtract_7 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                           U16x16( 8, 9,10,11,12,13,14,15,16,         17,18,19,20,21,22,23)));
	ASSERT(allLanesEqual(vectorExtract_8 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                              U16x16( 9,10,11,12,13,14,15,16,         17,18,19,20,21,22,23,24)));
	ASSERT(allLanesEqual(vectorExtract_9 (U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                 U16x16(10,11,12,13,14,15,16,         17,18,19,20,21,22,23,24,25)));
	ASSERT(allLanesEqual(vectorExtract_10(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                    U16x16(11,12,13,14,15,16,         17,18,19,20,21,22,23,24,25,26)));
	ASSERT(allLanesEqual(vectorExtract_11(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                       U16x16(12,13,14,15,16,         17,18,19,20,21,22,23,24,25,26,27)));
	ASSERT(allLanesEqual(vectorExtract_12(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                          U16x16(13,14,15,16,         17,18,19,20,21,22,23,24,25,26,27,28)));
	ASSERT(allLanesEqual(vectorExtract_13(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                             U16x16(14,15,16,         17,18,19,20,21,22,23,24,25,26,27,28,29)));
	ASSERT(allLanesEqual(vectorExtract_14(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                                U16x16(15,16,         17,18,19,20,21,22,23,24,25,26,27,28,29,30)));
	ASSERT(allLanesEqual(vectorExtract_15(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                                   U16x16(16,         17,18,19,20,21,22,23,24,25,26,27,28,29,30,31)));
	ASSERT(allLanesEqual(vectorExtract_16(U16x16( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16), U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)),
	                                                                                               U16x16(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)));
	ASSERT(allLanesEqual(vectorExtract_0 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                      U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32)));
	ASSERT(allLanesEqual(vectorExtract_1 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                      U8x32( 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33)));
	ASSERT(allLanesEqual(vectorExtract_2 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                            U8x32( 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34)));
	ASSERT(allLanesEqual(vectorExtract_3 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                               U8x32( 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35)));
	ASSERT(allLanesEqual(vectorExtract_4 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                  U8x32( 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36)));
	ASSERT(allLanesEqual(vectorExtract_5 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                     U8x32( 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37)));
	ASSERT(allLanesEqual(vectorExtract_6 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                        U8x32( 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38)));
	ASSERT(allLanesEqual(vectorExtract_7 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                           U8x32( 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39)));
	ASSERT(allLanesEqual(vectorExtract_8 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                              U8x32( 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40)));
	ASSERT(allLanesEqual(vectorExtract_9 (U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                 U8x32(10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41)));
	ASSERT(allLanesEqual(vectorExtract_10(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                    U8x32(11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42)));
	ASSERT(allLanesEqual(vectorExtract_11(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                       U8x32(12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43)));
	ASSERT(allLanesEqual(vectorExtract_12(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                          U8x32(13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44)));
	ASSERT(allLanesEqual(vectorExtract_13(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                             U8x32(14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45)));
	ASSERT(allLanesEqual(vectorExtract_14(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                U8x32(15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46)));
	ASSERT(allLanesEqual(vectorExtract_15(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                   U8x32(16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47)));
	ASSERT(allLanesEqual(vectorExtract_16(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                      U8x32(17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48)));
	ASSERT(allLanesEqual(vectorExtract_17(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                         U8x32(18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49)));
	ASSERT(allLanesEqual(vectorExtract_18(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                            U8x32(19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50)));
	ASSERT(allLanesEqual(vectorExtract_19(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                               U8x32(20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51)));
	ASSERT(allLanesEqual(vectorExtract_20(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                  U8x32(21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52)));
	ASSERT(allLanesEqual(vectorExtract_21(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                     U8x32(22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53)));
	ASSERT(allLanesEqual(vectorExtract_22(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                        U8x32(23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54)));
	ASSERT(allLanesEqual(vectorExtract_23(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                           U8x32(24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55)));
	ASSERT(allLanesEqual(vectorExtract_24(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                              U8x32(25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56)));
	ASSERT(allLanesEqual(vectorExtract_25(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                 U8x32(26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57)));
	ASSERT(allLanesEqual(vectorExtract_26(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                    U8x32(27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58)));
	ASSERT(allLanesEqual(vectorExtract_27(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                       U8x32(28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59)));
	ASSERT(allLanesEqual(vectorExtract_28(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                          U8x32(29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60)));
	ASSERT(allLanesEqual(vectorExtract_29(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                             U8x32(30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61)));
	ASSERT(allLanesEqual(vectorExtract_30(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                                U8x32(31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62)));
	ASSERT(allLanesEqual(vectorExtract_31(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                                           U8x32(32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63)));
	ASSERT(allLanesEqual(vectorExtract_32(U8x32( 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32), U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)),
	                                                                                                                                              U8x32(33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64)));
	{ // Gather test
		// The Buffer must be kept alive during the pointer's lifetime to prevent freeing the memory too early with reference counting.
		//   Because SafePointer exists only to be faster than Buffer but safer than a raw pointer.
		Buffer gatherTestBuffer = buffer_create(sizeof(int32_t) * 32);
		{
			// 32-bit floating-point gather
			SafePointer<float> pointerF = buffer_getSafeData<float>(gatherTestBuffer, "float gather test data");
			for (int i = 0; i < 32; i++) { // -32.0f, -30.0f, -28.0f, -26.0f ... 24.0f, 26.0f, 28.0f, 30.0f
				pointerF[i] = i * 2.0f - 32.0f;
			}
			ASSERT(allLanesEqual(gather_F32(pointerF     , U32x4(2, 1, 30, 31)), F32x4(-28.0f, -30.0f, 28.0f, 30.0f)));
			ASSERT(allLanesEqual(gather_F32(pointerF + 10, U32x4(0, 1, 2, 3)), F32x4(-12.0f, -10.0f, -8.0f, -6.0f)));
			ASSERT(allLanesEqual(gather_F32(pointerF     , U32x8(2, 1, 28, 29, 3, 0, 30, 31)), F32x8(-28.0f, -30.0f, 24.0f, 26.0f, -26.0f, -32.0f, 28.0f, 30.0f)));
			ASSERT(allLanesEqual(gather_F32(pointerF + 10, U32x8(0, 1, 2, 3, 4, 5, 6, 7)), F32x8(-12.0f, -10.0f, -8.0f, -6.0f, -4.0f, -2.0f, 0.0f, 2.0f)));
		}
		{
			// Signed 32-bit integer gather
			SafePointer<int32_t> pointerU = buffer_getSafeData<int32_t>(gatherTestBuffer, "int32_t gather test data");
			for (int i = 0; i < 32; i++) { // -32, -30, -28, -26 ... 24, 26, 28, 30
				pointerU[i] = i * 2 - 32;
			}
			ASSERT(allLanesEqual(gather_I32(pointerU     , U32x4(2, 1, 30, 31)), I32x4(-28, -30, 28, 30)));
			ASSERT(allLanesEqual(gather_I32(pointerU + 10, U32x4(0, 1, 2, 3)), I32x4(-12, -10, -8, -6)));
			ASSERT(allLanesEqual(gather_I32(pointerU     , U32x8(2, 1, 28, 29, 3, 0, 30, 31)), I32x8(-28, -30, 24, 26, -26, -32, 28, 30)));
			ASSERT(allLanesEqual(gather_I32(pointerU + 10, U32x8(0, 1, 2, 3, 4, 5, 6, 7)), I32x8(-12, -10, -8, -6, -4, -2, 0, 2)));
		}
		{
			// Unsigned 32-bit integer gather
			SafePointer<uint32_t> pointerI = buffer_getSafeData<uint32_t>(gatherTestBuffer, "uint32_t gather test data");
			for (int i = 0; i < 32; i++) { // 100, 102, 104, 106 ... 156, 158, 160, 162
				pointerI[i] = 100 + i * 2;
			}
			// Signed 32-bit integer gather
			ASSERT(allLanesEqual(gather_U32(pointerI     , U32x4(2, 1, 30, 31)), U32x4(104, 102, 160, 162)));
			ASSERT(allLanesEqual(gather_U32(pointerI + 10, U32x4(0, 1, 2, 3)), U32x4(120, 122, 124, 126)));
			ASSERT(allLanesEqual(gather_U32(pointerI     , U32x8(2, 1, 28, 29, 3, 0, 30, 31)), U32x8(104, 102, 156, 158, 106, 100, 160, 162)));
			ASSERT(allLanesEqual(gather_U32(pointerI + 10, U32x8(0, 1, 2, 3, 4, 5, 6, 7)), U32x8(120, 122, 124, 126, 128, 130, 132, 134)));
		}
	}

END_TEST
