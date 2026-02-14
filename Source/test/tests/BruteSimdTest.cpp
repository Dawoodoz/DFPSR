
#include "../testTools.h"
#include "../../DFPSR/base/simd.h"
#include "../../DFPSR/base/TemporaryCallback.h"
#include "../../DFPSR/api/randomAPI.h"

static const intptr_t ITERATIONS = 1000000;

template<typename T>
T generate(RandomGenerator &generator) {
	return (T)random_generate_U64(generator);
}
template<>
float generate<float>(RandomGenerator &generator) {
	// Too big floats will fail from not having enough precision, so this random generator is limited within -1000.0f to 1000.0f.
	int32_t fractions = random_generate_range(generator, -1000000, 1000000);
	return float(fractions) * 0.001f;
}

template<typename T>
bool somewhatEqual(const T &a, const T &b) {
	double da = double(a);
	double db = double(b);
	return (da < db + 0.0001) && (da > db - 0.0001);
}

template<typename S_IN, typename S_OUT, typename V_IN, typename V_OUT>
void unaryEquivalent (
  const TemporaryCallback<S_OUT(const S_IN &a)> &scalarOp,
  const TemporaryCallback<V_OUT(const V_IN &a)> &simdOp,
  const ReadableString &testName
) {
	constexpr intptr_t laneCount = (sizeof(V_IN) / sizeof(S_IN));
	// This test only applies to functions where input and output has the same number of lanes.
	ASSERT_EQUAL(laneCount, sizeof(V_OUT) / sizeof(S_OUT));
	// Initialize a random generator independently of other bruteforce tests
	//   so that disabling another test does not affect this test.
	RandomGenerator generator = random_createGenerator(460983751);
	// Loop over random inputs.
	for (intptr_t iteration = 0; iteration < ITERATIONS; iteration++) {
		// Generate random input.
		ALIGN_BYTES(sizeof(V_IN)) S_IN inputA[laneCount];
		for (intptr_t lane = 0; lane < laneCount; lane++) {
			inputA[lane] = generate<S_IN>(generator);
		}
		// Execute scalar operation for all lanes.
		ALIGN_BYTES(sizeof(V_OUT)) S_OUT scalarResult[laneCount];
		for (intptr_t lane = 0; lane < laneCount; lane++) {
			scalarResult[lane] = scalarOp(inputA[lane]);
		}
		// Execute SIMD operation with all lanes at the same time.
		V_IN simdInputA = V_IN::readAlignedUnsafe(inputA);
		V_OUT simdOutput = simdOp(simdInputA);
		ALIGN_BYTES(sizeof(V_OUT)) S_OUT vectorResult[laneCount];
		simdOutput.writeAlignedUnsafe(vectorResult);
		// Compare results.
		for (intptr_t lane = 0; lane < laneCount; lane++) {
			if (!somewhatEqual(scalarResult[lane], vectorResult[lane])) {
				printText(U"\n_______________________________ FAIL _______________________________\n");
				printText(U"Wrong result at lane ", lane, U" of 0..", laneCount - 1, U" at iteration ", iteration, U" of ", testName, U"!\n");
				printText(U"Input: ", inputA[lane], U"\n");
				printText(U"Scalar result: ", scalarResult[lane], U"\n");
				printText(U"Vector result: ", vectorResult[lane], U"\n");
				printText(U"\n____________________________________________________________________\n");
				failed = true;
				return;
			}
		}
	}
	printText(U"*");
}

template<typename S_IN, typename S_OUT, typename V_IN, typename V_OUT>
void binaryEquivalent (
  const TemporaryCallback<S_OUT(const S_IN &a, const S_IN &b)> &scalarOp,
  const TemporaryCallback<V_OUT(const V_IN &a, const V_IN &b)> &simdOp,
  const ReadableString &testName
) {
	constexpr intptr_t laneCount = (sizeof(V_IN) / sizeof(S_IN));
	// This test only applies to functions where input and output has the same number of lanes.
	ASSERT_EQUAL(laneCount, sizeof(V_OUT) / sizeof(S_OUT));
	// Initialize a random generator independently of other bruteforce tests
	//   so that disabling another test does not affect this test.
	RandomGenerator generator = random_createGenerator(460983751);
	// Loop over random inputs.
	for (intptr_t iteration = 0; iteration < ITERATIONS; iteration++) {
		// Generate random input.
		ALIGN_BYTES(sizeof(V_OUT)) S_IN inputA[laneCount];
		ALIGN_BYTES(sizeof(V_OUT)) S_IN inputB[laneCount];
		for (intptr_t lane = 0; lane < laneCount; lane++) {
			inputA[lane] = generate<S_IN>(generator);
			inputB[lane] = generate<S_IN>(generator);
		}
		// Execute scalar operation for all lanes.
		ALIGN_BYTES(sizeof(V_OUT)) S_OUT scalarResult[laneCount];
		for (intptr_t lane = 0; lane < laneCount; lane++) {
			scalarResult[lane] = scalarOp(inputA[lane], inputB[lane]);
		}
		// Execute SIMD operation with all lanes at the same time.
		V_IN simdInputA = V_IN::readAlignedUnsafe(inputA);
		V_IN simdInputB = V_IN::readAlignedUnsafe(inputB);
		V_OUT simdOutput = simdOp(simdInputA, simdInputB);
		ALIGN_BYTES(sizeof(V_OUT)) S_OUT vectorResult[laneCount];
		simdOutput.writeAlignedUnsafe(vectorResult);
		// Compare results.
		for (intptr_t lane = 0; lane < laneCount; lane++) {
			if (!somewhatEqual(scalarResult[lane], vectorResult[lane])) {
				printText(U"\n_______________________________ FAIL _______________________________\n");
				printText(U"\nWrong result at lane ", lane, U" of 0..", laneCount - 1, U" at iteration ", iteration, U" of ", testName, U"!\n");
				printText(U"Input: ", inputA[lane], U", ", inputB[lane], U"\n");
				printText(U"Scalar result: ", scalarResult[lane], U"\n");
				printText(U"Vector result: ", vectorResult[lane], U"\n");
				printText(U"\n____________________________________________________________________\n");
				failed = true;
				return;
			}
		}
	}
	printText(U"*");
}

#define UNARY_POINT_EQUIVALENCE(S, V, EXPR) unaryEquivalent<S, S, V, V>([](const S &a) -> S { return EXPR; }, [](const V &a) -> V { return EXPR; }, U"unary function equivalence test between " U###S U" and " U###V U" for " U###EXPR)
#define BINARY_POINT_EQUIVALENCE(S, V, EXPR) binaryEquivalent<S, S, V, V>([](const S &a, const S &b) -> S { return EXPR; }, [](const V &a, const V &b) -> V { return EXPR; }, U"binary function equivalence test between " U###S U" and " U###V U" for " U###EXPR)

START_TEST(BruteSimd)
	printText(U"\nThe bruteforce SIMD test is compiled using:\n");
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

	// Addition.
	BINARY_POINT_EQUIVALENCE(uint8_t , U8x16 , a + b);
	BINARY_POINT_EQUIVALENCE(uint8_t , U8x32 , a + b);
	BINARY_POINT_EQUIVALENCE(uint16_t, U16x8 , a + b);
	BINARY_POINT_EQUIVALENCE(uint16_t, U16x16, a + b);
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x4 , a + b);
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x8 , a + b);
	BINARY_POINT_EQUIVALENCE(int32_t , I32x4 , a + b);
	BINARY_POINT_EQUIVALENCE(int32_t , I32x4 , a + b);
	BINARY_POINT_EQUIVALENCE(float   , F32x4 , a + b);
	BINARY_POINT_EQUIVALENCE(float   , F32x4 , a + b);

	// Subtraction
	BINARY_POINT_EQUIVALENCE(uint8_t , U8x16 , a - b);
	BINARY_POINT_EQUIVALENCE(uint8_t , U8x32 , a - b);
	BINARY_POINT_EQUIVALENCE(uint16_t, U16x8 , a - b);
	BINARY_POINT_EQUIVALENCE(uint16_t, U16x16, a - b);
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x4 , a - b);
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x8 , a - b);
	BINARY_POINT_EQUIVALENCE(int32_t , I32x4 , a - b);
	BINARY_POINT_EQUIVALENCE(int32_t , I32x4 , a - b);
	BINARY_POINT_EQUIVALENCE(float   , F32x4 , a - b);
	BINARY_POINT_EQUIVALENCE(float   , F32x4 , a - b);

	// Negation
	UNARY_POINT_EQUIVALENCE(int32_t , I32x4 , -a);
	UNARY_POINT_EQUIVALENCE(int32_t , I32x4 , -a);
	UNARY_POINT_EQUIVALENCE(float   , F32x4 , -a);
	UNARY_POINT_EQUIVALENCE(float   , F32x4 , -a);

	// Multiplication
	//BINARY_POINT_EQUIVALENCE(uint8_t , U8x16 , a * b); // Missing
	//BINARY_POINT_EQUIVALENCE(uint8_t , U8x32 , a * b); // Missing
	//BINARY_POINT_EQUIVALENCE(uint16_t, U16x8 , a * b); // Missing
	//BINARY_POINT_EQUIVALENCE(uint16_t, U16x16, a * b); // Missing
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x4 , a * b);
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x8 , a * b);
	BINARY_POINT_EQUIVALENCE(int32_t , I32x4 , a * b);
	BINARY_POINT_EQUIVALENCE(int32_t , I32x4 , a * b);
	BINARY_POINT_EQUIVALENCE(float   , F32x4 , a * b);
	BINARY_POINT_EQUIVALENCE(float   , F32x4 , a * b);

	// Bitwise and
	//BINARY_POINT_EQUIVALENCE(uint8_t , U8x16 , a & b); // Missing
	//BINARY_POINT_EQUIVALENCE(uint8_t , U8x32 , a & b); // Missing
	BINARY_POINT_EQUIVALENCE(uint16_t, U16x8 , a & b);
	BINARY_POINT_EQUIVALENCE(uint16_t, U16x16, a & b);
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x4 , a & b);
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x8 , a & b);

	// Bitwise or
	//BINARY_POINT_EQUIVALENCE(uint8_t , U8x16, a | b); // Missing
	//BINARY_POINT_EQUIVALENCE(uint8_t , U8x32, a | b); // Missing
	BINARY_POINT_EQUIVALENCE(uint16_t, U16x8 , a | b);
	BINARY_POINT_EQUIVALENCE(uint16_t, U16x16, a | b);
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x4 , a | b);
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x8 , a | b);

	// Bitwise xor
	//BINARY_POINT_EQUIVALENCE(uint8_t , U8x16, a ^ b); // Missing
	//BINARY_POINT_EQUIVALENCE(uint8_t , U8x32, a ^ b); // Missing
	BINARY_POINT_EQUIVALENCE(uint16_t, U16x8 , a ^ b);
	BINARY_POINT_EQUIVALENCE(uint16_t, U16x16, a ^ b);
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x4 , a ^ b);
	BINARY_POINT_EQUIVALENCE(uint32_t, U32x8 , a ^ b);

	// Bitwise negation
	UNARY_POINT_EQUIVALENCE(uint16_t, U16x8 , ~a);
	UNARY_POINT_EQUIVALENCE(uint16_t, U16x16, ~a);
	UNARY_POINT_EQUIVALENCE(uint32_t, U32x4 , ~a);
	UNARY_POINT_EQUIVALENCE(uint32_t, U32x8 , ~a);

END_TEST
