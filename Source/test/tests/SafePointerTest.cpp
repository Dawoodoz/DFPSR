
#include "../testTools.h"
#include "../../DFPSR/base/SafePointer.h"

START_TEST(SafePointer)
	// Simulate unaligned memory
	const int elements = 100;
	const int dataSize = elements * sizeof(int32_t);
	const int alignment = 16;
	const int bufferSize = dataSize + alignment - 1;
	uint8_t allocation[3][bufferSize];
	// Run the algorithm for each byte offset relative to the alignment
	for (int offset = 0; offset < alignment; offset++) {
		// The SafePointer should be inlined into a raw pointer in relase mode while providing full bound checks in debug mode.
		SafePointer<int32_t> bufferA("bufferA", (int32_t*)(allocation[0] + offset), dataSize);
		SafePointer<int32_t> bufferB("bufferB", (int32_t*)(allocation[1] + offset), dataSize);
		SafePointer<int32_t> bufferC("bufferC", (int32_t*)(allocation[2] + offset), dataSize);
		// Initialize
		for (int i = 0; i < elements; i++) {
			bufferA[i] = i % 13;
			bufferB[i] = i % 7;
			bufferC[i] = 0;
		}
		// Calculate
		SafePointer<const int32_t> readerA = bufferA;
		SafePointer<const int32_t> readerB = bufferB;
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
		// Make sure that array bounds are tested if they are turned on using the debug mode
		#ifdef SAFE_POINTER_CHECKS
			ASSERT_CRASH(bufferC[-1], U"SafePointer out of bound exception!");
			ASSERT_CRASH(bufferB[-65], U"SafePointer out of bound exception!");
			ASSERT_CRASH(bufferA[-245654], U"SafePointer out of bound exception!");
			ASSERT_CRASH(bufferA[elements], U"SafePointer out of bound exception!");
			ASSERT_CRASH(bufferA[elements + 1], U"SafePointer out of bound exception!");
			ASSERT_CRASH(bufferB[elements + 23], U"SafePointer out of bound exception!");
			ASSERT_CRASH(bufferC[elements + 673578], U"SafePointer out of bound exception!");
		#endif
	}
	#ifndef SAFE_POINTER_CHECKS
		#error "ERROR! SafePointer test ran without bound checks enabled!\n"
	#endif
END_TEST
