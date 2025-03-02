
#include "../testTools.h"
#include "../../DFPSR/api/randomAPI.h"

START_TEST(Random)
	{
		RandomGenerator generator = random_createGenerator(123456789u);
		// Generate ten million values and add them to a histogram.
		uint64_t histogram[100] = {};
		for (int32_t v = 0; v < 10000000; v++) {
			int64_t result = random_generate_range(generator, 0, 99);
			histogram[result]++;
		}
		// Check that none of the values occurs more than 2% from the expected average.
		for (int32_t h = 0; h < 100; h++) {
			ASSERT_LESSER(histogram[h], 102000);
			ASSERT_GREATER(histogram[h], 98000);
		}
	}
	{
		RandomGenerator generator = random_createGenerator(4857623u);

		// 0% probability
		uint64_t trueCount = 0u; uint64_t falseCount = 0u;
		for (int32_t h = 0; h < 10000000; h++) {
			if (random_generate_probability(generator, 0)) {
				trueCount++;
			} else {
				falseCount++;
			}
		}
		ASSERT_EQUAL(trueCount, 0u);
		ASSERT_EQUAL(falseCount, 10000000u);

		// Out of bound probability
		trueCount = 0u; falseCount = 0u;
		for (int32_t h = 0; h < 10000000; h++) {
			if (random_generate_probability(generator, -25)) {
				trueCount++;
			} else {
				falseCount++;
			}
		}
		ASSERT_EQUAL(trueCount, 0u);
		ASSERT_EQUAL(falseCount, 10000000u);

		// 100% probability
		trueCount = 0u; falseCount = 0u;
		for (int32_t h = 0; h < 10000000; h++) {
			if (random_generate_probability(generator, 100)) {
				trueCount++;
			} else {
				falseCount++;
			}
		}
		ASSERT_EQUAL(trueCount, 10000000u);
		ASSERT_EQUAL(falseCount, 0u);

		// Out of bound probability
		trueCount = 0u; falseCount = 0u;
		for (int32_t h = 0; h < 10000000; h++) {
			if (random_generate_probability(generator, 125)) {
				trueCount++;
			} else {
				falseCount++;
			}
		}
		ASSERT_EQUAL(trueCount, 10000000u);
		ASSERT_EQUAL(falseCount, 0u);

		// 50% probability
		trueCount = 0u; falseCount = 0u;
		for (int32_t h = 0; h < 10000000; h++) {
			if (random_generate_probability(generator, 50)) {
				trueCount++;
			} else {
				falseCount++;
			}
		}
		ASSERT_GREATER(trueCount , 4990000u);
		ASSERT_GREATER(falseCount, 4990000u);
	}
	{
		// Making sure that the random generator does not break backward compatibility by changing its behavior.
		RandomGenerator generator = random_createGenerator(1223334444u);
		List<int32_t> generatedValues;
		for (int32_t h = 0; h < 100; h++) {
			generatedValues.push(random_generate_range(generator, -200, 200));
		}
		ASSERT_EQUAL(generatedValues,
		  List<int32_t>(
		    -192, -196, -106, -134, -72, -43, 52, -113, 51, 39, -29, 25, -2, 91, -109, 56, -17, -80, -59, 6, 185,
			-18, 102, 137, 166, -188, 130, -41, -100, -29, 160, 68, -171, -84, -76, 27, -151, -168, -91, 171, 155,
			-139, 46, 185, -140, -60, -173, 0, 81, -73, 36, -33, 145, -31, 73, 152, -107, -140, -63, 181, 176, -142,
			-122, 97, 102, 151, -110, 19, 103, -78, 21, -82, -89, -77, -69, -14, 27, -24, 6, 94, 186, -185, -71,
			-184, 127, -97, 173, -179, 70, -74, 13, 3, 11, 129, 116, -58, 35, -175, 116, -69));
	}
END_TEST
