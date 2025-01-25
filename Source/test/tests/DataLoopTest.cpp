
#include "../testTools.h"
#include "../../DFPSR/base/simd.h"

#define INITIALIZE \
	for (int i = 0; i < elements; i++) { \
		bufferA[i] = i % 13; \
		bufferB[i] = i % 7; \
		bufferC[i] = 0; \
	}

#define CHECK_RESULT \
	{ \
		int errors = 0; \
		for (int i = 0; i < elements; i++) { \
			if (bufferC[i] != ((i % 13) * (i % 7)) + 5) { \
				errors++; \
			} \
		} \
		ASSERT(errors == 0); \
	}

START_TEST(DataLoop)
	// Allocate aligned memory
	const int elements = 256;
	int32_t allocationA[elements] ALIGN16;
	int32_t allocationB[elements] ALIGN16;
	int32_t allocationC[elements] ALIGN16;
	// The SafePointer class will emulate the behaviour of a raw data pointer while providing full bound checks in debug mode.
	SafePointer<int32_t> bufferA("bufferA", allocationA, sizeof(allocationA));
	SafePointer<int32_t> bufferB("bufferB", allocationB, sizeof(allocationB));
	SafePointer<int32_t> bufferC("bufferC", allocationC, sizeof(allocationC));

	// Calculate C = (A * B) + 5

	{ // Reference implementation
		INITIALIZE;
		for (int i = 0; i < elements; i++) {
			bufferC[i] = (bufferA[i] * bufferB[i]) + 5;
		}
		CHECK_RESULT;
	}

	{ // Pointer version
		INITIALIZE;
		// Iterating pointers are created from the main pointers
		SafePointer<int32_t> ptrA = bufferA;
		SafePointer<int32_t> ptrB = bufferB;
		SafePointer<int32_t> ptrC = bufferC;
		for (int i = 0; i < elements; i++) {
			// *a is the same as a[0]
			*ptrC = *ptrA * *ptrB + 5;
			// Adding one to a pointer adds the size of the element type
			ptrA += 1;
			ptrB += 1;
			ptrC += 1;
		}
		CHECK_RESULT;
	}

	{ // Pseudo vectorization (what the SIMD math actually means)
		INITIALIZE;
		for (int i = 0; i < elements - 3; i += 4) {
			bufferC[i + 0] = (bufferA[i + 0] * bufferB[i + 0]) + 5;
			bufferC[i + 1] = (bufferA[i + 1] * bufferB[i + 1]) + 5;
			bufferC[i + 2] = (bufferA[i + 2] * bufferB[i + 2]) + 5;
			bufferC[i + 3] = (bufferA[i + 3] * bufferB[i + 3]) + 5;
		}
		CHECK_RESULT;
	}

	{ // SIMD version
		INITIALIZE;
		SafePointer<int32_t> ptrA = bufferA;
		SafePointer<int32_t> ptrB = bufferB;
		SafePointer<int32_t> ptrC = bufferC;
		for (int i = 0; i < elements - 3; i += 4) {
			// Read vectors 
			I32x4 a = I32x4::readAligned(ptrA, "data loop test @ read a");
			I32x4 b = I32x4::readAligned(ptrB, "data loop test @ read b");
			// Do the calculation with 4 elements at once from i to i + 3 using SIMD operations
			I32x4 result = a * b + 5;
			// Write the result
			result.writeAligned(ptrC, "data loop test @ write c");
			// Iterate pointers
			ptrA += 4;
			ptrB += 4;
			ptrC += 4;
		}
		CHECK_RESULT;
	}



END_TEST

