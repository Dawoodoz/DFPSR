
#include "../testTools.h"

START_TEST(SafePointer)
	// Simulate unaligned memory
	const int elements = 100;
	const int dataSize = elements * sizeof(int32_t);
	const int alignment = 16;
	const int bufferSize = dataSize + alignment - 1;
	uint8_t allocation[3][bufferSize];
	// Run the algorithm for each byte offset relative to the alignment
	for (int offset = 0; offset < alignment; offset++) {
		// The SafePointer class will emulate the behaviour of a raw data pointer while providing full bound checks in debug mode.
		SafePointer<int32_t> bufferA("bufferA", (int32_t*)(allocation[0] + offset), dataSize);
		SafePointer<int32_t> bufferB("bufferB", (int32_t*)(allocation[1] + offset), dataSize);
		SafePointer<int32_t> bufferC("bufferC", (int32_t*)(allocation[2] + offset), dataSize);
		// Make sure that array bounds are tested if they are turned on using the debug mode
		#ifdef SAFE_POINTER_CHECKS
			ASSERT_CRASH(bufferA[-245654]);
			ASSERT_CRASH(bufferB[-65]);
			ASSERT_CRASH(bufferC[-1]);
			ASSERT_CRASH(bufferA[elements]);
			ASSERT_CRASH(bufferB[elements + 23]);
			ASSERT_CRASH(bufferC[elements + 673578]);
		#endif
		// Initialize
		for (int i = 0; i < elements; i++) {
			bufferA[i] = i % 13;
			bufferB[i] = i % 7;
			bufferC[i] = 0;
		}
		// Calculate
		const SafePointer<int32_t> readerA = bufferA;
		const SafePointer<int32_t> readerB = bufferB;
		for (int i = 0; i < elements; i++) {
			bufferC[i] = (*readerA * *readerB) + 5;
			readerA += 1; readerB += 1;
		}
		// Check results
		int errors = 0;
		for (int i = 0; i < elements; i++) {
			if (bufferC[i] != ((i % 13) * (i % 7)) + 5) {
				errors++;
			}
		}
		ASSERT(errors == 0);
	}
	#ifndef SAFE_POINTER_CHECKS
		printf("WARNING! SafePointer test ran without bound checks enabled.\n");
	#endif
END_TEST

