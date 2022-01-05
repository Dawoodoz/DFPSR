
#include "../testTools.h"
#include "../../DFPSR/base/simdExtra.h"

START_TEST(Simd)
	printText("\nSIMD test is compiled using:\n");
	#ifdef USE_SSE2
		printText("	* SSE2\n");
	#endif
	#ifdef USE_SSSE3
		printText("	* SSSE3\n");
	#endif
	#ifdef USE_AVX2
		printText("	* AVX2\n");
	#endif
	#ifdef USE_NEON
		printText("	* NEON\n");
	#endif

	// F32x4 Comparisons
	ASSERT_EQUAL(F32x4(1.5f), F32x4(1.5f, 1.5f, 1.5f, 1.5f));
	ASSERT_EQUAL(F32x4(-1.5f), F32x4(-1.5f, -1.5f, -1.5f, -1.5f));
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, 7.8f));
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().x, 1.2f);
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().y, 3.4f);
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().z, 5.6f);
	ASSERT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f).get().w, 7.8f);
	ASSERT_NOT_EQUAL(F32x4(1.3f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, 7.8f));
	ASSERT_NOT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, -1.4f, 5.6f, 7.8f));
	ASSERT_NOT_EQUAL(F32x4(1.2f, 3.4f, 5.5f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, 7.8f));
	ASSERT_NOT_EQUAL(F32x4(1.2f, 3.4f, 5.6f, 7.8f), F32x4(1.2f, 3.4f, 5.6f, -7.8f));

	// I32x4 Comparisons
	ASSERT_EQUAL(I32x4(4), I32x4(4, 4, 4, 4));
	ASSERT_EQUAL(I32x4(-4), I32x4(-4, -4, -4, -4));
	ASSERT_EQUAL(I32x4(-1, 2, -3, 4), I32x4(-1, 2, -3, 4));
	ASSERT_NOT_EQUAL(I32x4(-1, 2, 7, 4), I32x4(-1, 2, -3, 4));

	// U32x4 Comparisons
	ASSERT_EQUAL(U32x4(4), U32x4(4, 4, 4, 4));
	ASSERT_EQUAL(U32x4(1, 2, 3, 4), U32x4(1, 2, 3, 4));
	ASSERT_NOT_EQUAL(U32x4(1, 2, 7, 4), U32x4(1, 2, 3, 4));

	// U16x8 Comparisons
	ASSERT_EQUAL(U16x8((uint16_t)8), U16x8(8, 8, 8, 8, 8, 8, 8, 8));
	ASSERT_EQUAL(U16x8((uint32_t)8), U16x8(8, 0, 8, 0, 8, 0, 8, 0));
	ASSERT_EQUAL(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(0, 2, 3, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(1, 0, 3, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(1, 2, 0, 4, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(1, 2, 3, 0, 5, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(1, 2, 3, 4, 0, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(1, 2, 3, 4, 5, 0, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(1, 2, 3, 4, 5, 6, 0, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(1, 2, 3, 4, 5, 6, 7, 0), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(1, 2, 0, 4, 5, 0, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(1, 0, 3, 4, 5, 6, 0, 0), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(0, 2, 3, 4, 0, 6, 7, 8), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_NOT_EQUAL(U16x8(0, 0, 0, 0, 0, 0, 0, 0), U16x8(1, 2, 3, 4, 5, 6, 7, 8));

	// U8x16 Comparisons
	ASSERT_EQUAL(U8x16((uint8_t)250), U8x16(250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250));
	ASSERT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 0, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 0, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 0, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 0, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 0, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 0, 8, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 0, 10, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 0, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 0, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 0, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 0, 255), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 0), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 251, 252, 6, 254, 255), U8x16(1, 2, 3, 4, 5, 9, 7, 8, 9, 10, 250, 251, 252, 253, 254, 255));
	ASSERT_NOT_EQUAL(U8x16(1, 2, 3, 0, 5, 6, 7, 8, 9, 0, 250, 251, 252, 253, 254, 255), U8x16(1, 2, 3, 4, 5, 6, 4, 8, 9, 10, 250, 251, 252, 253, 254, 255));

	// Macros
	#ifdef USE_BASIC_SIMD
		{ // Truncate float to int
			SIMD_F32x4 f = LOAD_VECTOR_F32_SIMD(-1.01f, -0.99f, 0.99f, 1.01f);
			SIMD_I32x4 i = LOAD_VECTOR_I32_SIMD(-1, 0, 0, 1);
			ASSERT_EQUAL(I32x4(F32_TO_I32_SIMD(f)), I32x4(i));
		}
		{ // Int to float
			SIMD_I32x4 n = LOAD_VECTOR_I32_SIMD(123   , 456   , 789   , -1000   );
			SIMD_F32x4 r = LOAD_VECTOR_F32_SIMD(123.0f, 456.0f, 789.0f, -1000.0f);
			ASSERT_EQUAL(F32x4(I32_TO_F32_SIMD(n)), F32x4(r));
		}
		{ // Signed-unsigned cast
			ASSERT_EQUAL(I32x4(REINTERPRET_U32_TO_I32_SIMD(U32x4(1, 2, 3, 4).v)), I32x4(1, 2, 3, 4));
			ASSERT_EQUAL(U32x4(REINTERPRET_I32_TO_U32_SIMD(I32x4(1, 2, 3, 4).v)), U32x4(1, 2, 3, 4));
		}
		{ // F32x4
			SIMD_F32x4 a = LOAD_VECTOR_F32_SIMD(-1.3f, 2.5f, -3.4f, 4.7f);
			SIMD_F32x4 b = LOAD_VECTOR_F32_SIMD(5.2f, -2.0f, 0.1f, 1.9f);
			SIMD_F32x4 c = LOAD_SCALAR_F32_SIMD(0.5f);
			ASSERT_EQUAL(F32x4(ADD_F32_SIMD(a, b)), F32x4(-1.3f + 5.2f, 2.5f + -2.0f, -3.4f + 0.1f, 4.7f + 1.9f));
			ASSERT_EQUAL(F32x4(SUB_F32_SIMD(a, b)), F32x4(-1.3f - 5.2f, 2.5f - -2.0f, -3.4f - 0.1f, 4.7f - 1.9f));
			ASSERT_EQUAL(F32x4(ADD_F32_SIMD(a, c)), F32x4(-1.3f + 0.5f, 2.5f + 0.5f, -3.4f + 0.5f, 4.7f + 0.5f));
			ASSERT_EQUAL(F32x4(SUB_F32_SIMD(a, c)), F32x4(-1.3f - 0.5f, 2.5f - 0.5f, -3.4f - 0.5f, 4.7f - 0.5f));
			ASSERT_EQUAL(F32x4(MUL_F32_SIMD(a, c)), F32x4(-1.3f * 0.5f, 2.5f * 0.5f, -3.4f * 0.5f, 4.7f * 0.5f));
			ASSERT_EQUAL(F32x4(MIN_F32_SIMD(a, b)), F32x4(-1.3f, -2.0f, -3.4f, 1.9f));
			ASSERT_EQUAL(F32x4(MAX_F32_SIMD(a, b)), F32x4(5.2f, 2.5f, 0.1f, 4.7f));
		}
		{ // I32x4
			SIMD_I32x4 a = LOAD_VECTOR_I32_SIMD(-1, 2, -3, 4);
			SIMD_I32x4 b = LOAD_VECTOR_I32_SIMD(5, -2, 0, 1);
			SIMD_I32x4 c = LOAD_SCALAR_I32_SIMD(4);
			ASSERT_EQUAL(I32x4(ADD_I32_SIMD(a, b)), I32x4(4, 0, -3, 5));
			ASSERT_EQUAL(I32x4(SUB_I32_SIMD(a, b)), I32x4(-6, 4, -3, 3));
			ASSERT_EQUAL(I32x4(ADD_I32_SIMD(a, c)), I32x4(3, 6, 1, 8));
			ASSERT_EQUAL(I32x4(SUB_I32_SIMD(a, c)), I32x4(-5, -2, -7, 0));
		}
		{ // U32x4
			SIMD_U32x4 a = LOAD_VECTOR_U32_SIMD(4, 5, 6, 7);
			SIMD_U32x4 b = LOAD_VECTOR_U32_SIMD(6, 5, 4, 3);
			SIMD_U32x4 c = LOAD_SCALAR_U32_SIMD(10);
			ASSERT_EQUAL(U32x4(ADD_U32_SIMD(a, b)), U32x4(c));
			ASSERT_EQUAL(U32x4(ADD_U32_SIMD(a, c)), U32x4(14, 15, 16, 17));
			ASSERT_EQUAL(U32x4(SUB_U32_SIMD(c, b)), U32x4(a));
		}
		{ // U16x8
			SIMD_U16x8 a = LOAD_VECTOR_U16_SIMD(1, 2, 3, 4, 5, 6, 7, 8);
			SIMD_U16x8 b = LOAD_VECTOR_U16_SIMD(9, 8, 7, 6, 5, 4, 3, 2);
			SIMD_U16x8 c = LOAD_SCALAR_U16_SIMD(10);
			ASSERT_EQUAL(U16x8(ADD_U16_SIMD(a, b)), U16x8(c));
			ASSERT_EQUAL(U16x8(ADD_U16_SIMD(a, c)), U16x8(11, 12, 13, 14, 15, 16, 17, 18));
			ASSERT_EQUAL(U16x8(SUB_U16_SIMD(c, b)), U16x8(a));
			ASSERT_EQUAL(U16x8(MUL_U16_SIMD(a, b)), U16x8(9, 16, 21, 24, 25, 24, 21, 16));
		}
	#endif

	// Reinterpret
	ASSERT_EQUAL(U16x8(U32x4(12, 34, 56, 78)), U16x8(12, 0, 34, 0, 56, 0, 78, 0));
	ASSERT_EQUAL(U16x8(12, 0, 34, 0, 56, 0, 78, 0).get_U32(), U32x4(12, 34, 56, 78));

	// Reciprocal: 1 / x
	ASSERT_EQUAL(F32x4(0.5f, 1.0f, 2.0f, 4.0f).reciprocal(), F32x4(2.0f, 1.0f, 0.5f, 0.25f));

	// Square root: sqrt(x)
	ASSERT_EQUAL(F32x4(1.0f, 4.0f, 9.0f, 100.0f).squareRoot(), F32x4(1.0f, 2.0f, 3.0f, 10.0f));

	// Reciprocal square root: 1 / sqrt(x)
	ASSERT_EQUAL(F32x4(1.0f, 4.0f, 16.0f, 100.0f).reciprocalSquareRoot(), F32x4(1.0f, 0.5f, 0.25f, 0.1f));

	// Minimum
	ASSERT_EQUAL(min(F32x4(1.1f, 2.2f, 3.3f, 4.4f), F32x4(5.0f, 3.0f, 1.0f, -1.0f)), F32x4(1.1f, 2.2f, 1.0f, -1.0f));

	// Maximum
	ASSERT_EQUAL(max(F32x4(1.1f, 2.2f, 3.3f, 4.4f), F32x4(5.0f, 3.0f, 1.0f, -1.0f)), F32x4(5.0f, 3.0f, 3.3f, 4.4f));

	// Clamp
	ASSERT_EQUAL(F32x4(-35.1f, 1.0f, 2.0f, 45.7f).clamp(-1.5f, 1.5f), F32x4(-1.5f, 1.0f, 1.5f, 1.5f));

	// F32x4 operations
	ASSERT_EQUAL(F32x4(1.1f, -2.2f, 3.3f, 4.0f) + F32x4(2.2f, -4.4f, 6.6f, 8.0f), F32x4(3.3f, -6.6f, 9.9f, 12.0f));
	ASSERT_EQUAL(F32x4(-1.5f, -0.5f, 0.5f, 1.5f) + 1.0f, F32x4(-0.5f, 0.5f, 1.5f, 2.5f));
	ASSERT_EQUAL(1.0f + F32x4(-1.5f, -0.5f, 0.5f, 1.5f), F32x4(-0.5f, 0.5f, 1.5f, 2.5f));
	ASSERT_EQUAL(F32x4(1.1f, 2.2f, 3.3f, 4.4f) - F32x4(0.1f, 0.2f, 0.3f, 0.4f), F32x4(1.0f, 2.0f, 3.0f, 4.0f));
	ASSERT_EQUAL(F32x4(1.0f, 2.0f, 3.0f, 4.0f) - 0.5f, F32x4(0.5f, 1.5f, 2.5f, 3.5f));
	ASSERT_EQUAL(0.5f - F32x4(1.0f, 2.0f, 3.0f, 4.0f), F32x4(-0.5f, -1.5f, -2.5f, -3.5f));
	ASSERT_EQUAL(2.0f * F32x4(1.0f, 2.0f, 3.0f, 4.0f), F32x4(2.0f, 4.0f, 6.0f, 8.0f));
	ASSERT_EQUAL(F32x4(1.0f, -2.0f, 3.0f, -4.0f) * -2.0f, F32x4(-2.0f, 4.0f, -6.0f, 8.0f));
	ASSERT_EQUAL(F32x4(1.0f, -2.0f, 3.0f, -4.0f) * F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(1.0f, 4.0f, 9.0f, 16.0f));

	// I32x4 operations
	ASSERT_EQUAL(I32x4(1, 2, -3, 4) + I32x4(-2, 4, 6, 8), I32x4(-1, 6, 3, 12));
	ASSERT_EQUAL(I32x4(1, -2, 3, 4) - 4, I32x4(-3, -6, -1, 0));
	ASSERT_EQUAL(10 + I32x4(1, 2, 3, 4), I32x4(11, 12, 13, 14));
	ASSERT_EQUAL(I32x4(1, 2, 3, 4) + I32x4(4), I32x4(5, 6, 7, 8));
	ASSERT_EQUAL(I32x4(10) + I32x4(1, 2, 3, 4), I32x4(11, 12, 13, 14));
	ASSERT_EQUAL(I32x4(-3, 6, -9, 12) * I32x4(1, 2, -3, -4), I32x4(-3, 12, 27, -48));

	// U32x4 operations
	ASSERT_EQUAL(U32x4(1, 2, 3, 4) + U32x4(2, 4, 6, 8), U32x4(3, 6, 9, 12));
	ASSERT_EQUAL(U32x4(1, 2, 3, 4) + 4, U32x4(5, 6, 7, 8));
	ASSERT_EQUAL(10 + U32x4(1, 2, 3, 4), U32x4(11, 12, 13, 14));
	ASSERT_EQUAL(U32x4(1, 2, 3, 4) + U32x4(4), U32x4(5, 6, 7, 8));
	ASSERT_EQUAL(U32x4(10) + U32x4(1, 2, 3, 4), U32x4(11, 12, 13, 14));
	ASSERT_EQUAL(U32x4(3, 6, 9, 12) - U32x4(1, 2, 3, 4), U32x4(2, 4, 6, 8));
	ASSERT_EQUAL(U32x4(3, 6, 9, 12) * U32x4(1, 2, 3, 4), U32x4(3, 12, 27, 48));

	// U16x8 operations
	ASSERT_EQUAL(U16x8(1, 2, 3, 4, 5, 6, 7, 8) + U16x8(2, 4, 6, 8, 10, 12, 14, 16), U16x8(3, 6, 9, 12, 15, 18, 21, 24));
	ASSERT_EQUAL(U16x8(1, 2, 3, 4, 5, 6, 7, 8) + 8, U16x8(9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_EQUAL(10 + U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(11, 12, 13, 14, 15, 16, 17, 18));
	ASSERT_EQUAL(U16x8(1, 2, 3, 4, 5, 6, 7, 8) + U16x8((uint16_t)8), U16x8(9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_EQUAL(U16x8((uint16_t)10) + U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(11, 12, 13, 14, 15, 16, 17, 18));
	ASSERT_EQUAL(U16x8(3, 6, 9, 12, 15, 18, 21, 24) - U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(2, 4, 6, 8, 10, 12, 14, 16));

	// U8x16 operations
	ASSERT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) + 2, U8x16(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));
	ASSERT_EQUAL(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) - 1, U8x16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
	ASSERT_EQUAL(
	  saturatedAddition(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 255), U8x16((uint8_t)250)),
	  U8x16(251, 252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255)
	);

	// Saturated unsigned integer packing
	ASSERT_EQUAL(saturateToU8(U16x8(1, 2, 3, 4, 65535, 6, 7, 8), U16x8(9, 10, 11, 12, 1000, 14, 15, 16)), U8x16(1, 2, 3, 4, 255, 6, 7, 8, 9, 10, 11, 12, 255, 14, 15, 16));

	// Unsigned integer unpacking
	ASSERT_EQUAL(lowerToU32(U16x8(1,2,3,4,5,6,7,8)), U32x4(1, 2, 3, 4));
	ASSERT_EQUAL(higherToU32(U16x8(1,2,3,4,5,6,7,8)), U32x4(5, 6, 7, 8));
	ASSERT_EQUAL(lowerToU16(U8x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)), U16x8(1,2,3,4,5,6,7,8));
	ASSERT_EQUAL(higherToU16(U8x16(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)), U16x8(9,10,11,12,13,14,15,16));

	// Reinterpretation
	ASSERT_EQUAL(
	  reinterpret_U8FromU32(U32x4(ENDIAN32_BYTE_0, ENDIAN32_BYTE_1, ENDIAN32_BYTE_2, ENDIAN32_BYTE_3)),
	  U8x16(
	    255, 0, 0, 0,
	    0, 255, 0, 0,
	    0, 0, 255, 0,
	    0, 0, 0, 255
	  )
	);
	ASSERT_EQUAL(
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
	ASSERT_EQUAL(
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

	// Bit mask
	ASSERT_EQUAL(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) & 0x0000FFFF, U32x4(0x0000FFFF, 0x00005678, 0x0000F0F0, 0x00000000));
	ASSERT_EQUAL(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) & 0xFFFF0000, U32x4(0xFFFF0000, 0x12340000, 0xF0F00000, 0x00000000));
	ASSERT_EQUAL(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) | 0x0000FFFF, U32x4(0xFFFFFFFF, 0x1234FFFF, 0xF0F0FFFF, 0x0000FFFF));
	ASSERT_EQUAL(U32x4(0xFFFFFFFF, 0x12345678, 0xF0F0F0F0, 0x00000000) | 0xFFFF0000, U32x4(0xFFFFFFFF, 0xFFFF5678, 0xFFFFF0F0, 0xFFFF0000));
	ASSERT_EQUAL(U32x4(0xFFFFFFFF, 0xFFF000FF, 0xF0F0F0F0, 0x12345678) & U32x4(0xFF00FF00, 0xFFFF0000, 0x000FF000, 0x0FF00FF0), U32x4(0xFF00FF00, 0xFFF00000, 0x0000F000, 0x02300670));
	ASSERT_EQUAL(U32x4(0xF00F000F, 0xFFF000FF, 0x10010011, 0xABC00000) | U32x4(0x0000FF00, 0xFFFF0000, 0x000FF000, 0x000DEF00), U32x4(0xF00FFF0F, 0xFFFF00FF, 0x100FF011, 0xABCDEF00));

	// Bit shift
	ASSERT_EQUAL(U32x4(1, 2, 3, 4) << 1, U32x4(2, 4, 6, 8));
	ASSERT_EQUAL(U32x4(1, 2, 3, 4) << 2, U32x4(4, 8, 12, 16));
	ASSERT_EQUAL(U32x4(1, 2, 3, 4) << 3, U32x4(8, 16, 24, 32));
	ASSERT_EQUAL(U32x4(1, 2, 3, 4) << 4, U32x4(16, 32, 48, 64));
	ASSERT_EQUAL(U32x4(1, 2, 3, 4) >> 1, U32x4(0, 1, 1, 2));
	ASSERT_EQUAL(U32x4(2, 4, 6, 8) >> 1, U32x4(1, 2, 3, 4));
	ASSERT_EQUAL(U32x4(2, 4, 6, 8) >> 2, U32x4(0, 1, 1, 2));
	ASSERT_EQUAL(U32x4(0x0AB12CD0, 0xFFFFFFFF, 0x12345678, 0xF0000000) << 4, U32x4(0xAB12CD00, 0xFFFFFFF0, 0x23456780, 0x00000000));
	ASSERT_EQUAL(U32x4(0x0AB12CD0, 0xFFFFFFFF, 0x12345678, 0x0000000F) >> 4, U32x4(0x00AB12CD, 0x0FFFFFFF, 0x01234567, 0x00000000));

	// Element shift with insert
	ASSERT_EQUAL(vectorExtract_0(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(1, 2, 3, 4));
	ASSERT_EQUAL(vectorExtract_1(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(2, 3, 4, 5));
	ASSERT_EQUAL(vectorExtract_2(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(3, 4, 5, 6));
	ASSERT_EQUAL(vectorExtract_3(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(4, 5, 6, 7));
	ASSERT_EQUAL(vectorExtract_4(U32x4(1, 2, 3, 4), U32x4(5, 6, 7, 8)), U32x4(5, 6, 7, 8));
	ASSERT_EQUAL(vectorExtract_0(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(123, 4294967295, 712, 45));
	ASSERT_EQUAL(vectorExtract_1(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(4294967295, 712, 45, 850514));
	ASSERT_EQUAL(vectorExtract_2(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(712, 45, 850514, 27));
	ASSERT_EQUAL(vectorExtract_3(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(45, 850514, 27, 0));
	ASSERT_EQUAL(vectorExtract_4(U32x4(123, 4294967295, 712, 45), U32x4(850514, 27, 0, 174)), U32x4(850514, 27, 0, 174));
	ASSERT_EQUAL(vectorExtract_0(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(1, 2, 3, 4));
	ASSERT_EQUAL(vectorExtract_1(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(2, 3, 4, 5));
	ASSERT_EQUAL(vectorExtract_2(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(3, 4, 5, 6));
	ASSERT_EQUAL(vectorExtract_3(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(4, 5, 6, 7));
	ASSERT_EQUAL(vectorExtract_4(I32x4(1, 2, 3, 4), I32x4(5, 6, 7, 8)), I32x4(5, 6, 7, 8));
	ASSERT_EQUAL(vectorExtract_0(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(123, 8462784, -712, 45));
	ASSERT_EQUAL(vectorExtract_1(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(8462784, -712, 45, -37562));
	ASSERT_EQUAL(vectorExtract_2(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(-712, 45, -37562, 27));
	ASSERT_EQUAL(vectorExtract_3(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(45, -37562, 27, 0));
	ASSERT_EQUAL(vectorExtract_4(I32x4(123, 8462784, -712, 45), I32x4(-37562, 27, 0, 174)), I32x4(-37562, 27, 0, 174));
	ASSERT_EQUAL(vectorExtract_0(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(1.0f, -2.0f, 3.0f, -4.0f));
	ASSERT_EQUAL(vectorExtract_1(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(-2.0f, 3.0f, -4.0f, 5.0f));
	ASSERT_EQUAL(vectorExtract_2(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(3.0f, -4.0f, 5.0f, 6.0f));
	ASSERT_EQUAL(vectorExtract_3(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(-4.0f, 5.0f, 6.0f, -7.0f));
	ASSERT_EQUAL(vectorExtract_4(F32x4(1.0f, -2.0f, 3.0f, -4.0f), F32x4(5.0f, 6.0f, -7.0f, 8.0f)), F32x4(5.0f, 6.0f, -7.0f, 8.0f));
	ASSERT_EQUAL(vectorExtract_0(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(1, 2, 3, 4, 5, 6, 7, 8));
	ASSERT_EQUAL(vectorExtract_1(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(2, 3, 4, 5, 6, 7, 8, 9));
	ASSERT_EQUAL(vectorExtract_2(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(3, 4, 5, 6, 7, 8, 9, 10));
	ASSERT_EQUAL(vectorExtract_3(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(4, 5, 6, 7, 8, 9, 10, 11));
	ASSERT_EQUAL(vectorExtract_4(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(5, 6, 7, 8, 9, 10, 11, 12));
	ASSERT_EQUAL(vectorExtract_5(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(6, 7, 8, 9, 10, 11, 12, 13));
	ASSERT_EQUAL(vectorExtract_6(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(7, 8, 9, 10, 11, 12, 13, 14));
	ASSERT_EQUAL(vectorExtract_7(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(8, 9, 10, 11, 12, 13, 14, 15));
	ASSERT_EQUAL(vectorExtract_8(U16x8(1, 2, 3, 4, 5, 6, 7, 8), U16x8(9, 10, 11, 12, 13, 14, 15, 16)), U16x8(9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_EQUAL(vectorExtract_0(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
	ASSERT_EQUAL(vectorExtract_1(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17));
	ASSERT_EQUAL(vectorExtract_2(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18));
	ASSERT_EQUAL(vectorExtract_3(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19));
	ASSERT_EQUAL(vectorExtract_4(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20));
	ASSERT_EQUAL(vectorExtract_5(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21));
	ASSERT_EQUAL(vectorExtract_6(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22));
	ASSERT_EQUAL(vectorExtract_7(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23));
	ASSERT_EQUAL(vectorExtract_8(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24));
	ASSERT_EQUAL(vectorExtract_9(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25));
	ASSERT_EQUAL(vectorExtract_10(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26));
	ASSERT_EQUAL(vectorExtract_11(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27));
	ASSERT_EQUAL(vectorExtract_12(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28));
	ASSERT_EQUAL(vectorExtract_13(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29));
	ASSERT_EQUAL(vectorExtract_14(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30));
	ASSERT_EQUAL(vectorExtract_15(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31));
	ASSERT_EQUAL(vectorExtract_16(U8x16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32)), U8x16(17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32));

	#ifdef USE_SIMD_EXTRA
		SIMD_U32x4 a = U32x4(1, 2, 3, 4).v;
		SIMD_U32x4 b = U32x4(5, 6, 7, 8).v;
		SIMD_U32x4x2 c = ZIP_U32_SIMD(a, b);
		ASSERT_EQUAL(U32x4(c.val[0]), U32x4(1, 5, 2, 6));
		ASSERT_EQUAL(U32x4(c.val[1]), U32x4(3, 7, 4, 8));
		SIMD_U32x4 d = ZIP_LOW_U32_SIMD(a, b);
		SIMD_U32x4 e = ZIP_HIGH_U32_SIMD(a, b);
		ASSERT_EQUAL(U32x4(d), U32x4(1, 5, 2, 6));
		ASSERT_EQUAL(U32x4(e), U32x4(3, 7, 4, 8));
	#endif
END_TEST

