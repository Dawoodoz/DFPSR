
#include "../testTools.h"
#include "../../DFPSR/base/virtualStack.h"
#include "../../DFPSR/base/threading.h"
#include <random>

inline int random(int min, int max) {
	return (std::rand() % (1 + max - min)) + min;
}

static bool bruteTest(int maxSize, int maxDepth) {
	// Select a random power of two as the alignment.
	uintptr_t alignment = 1 << random(0, 8);
	// The padded size does not have rounding requirements.
	uint64_t paddedSize = random(1, maxSize);
	uint64_t start = random(0, 255);
	uint64_t stride = random(0, 255);
	UnsafeAllocation allocation = virtualStack_push(paddedSize, memory_createAlignmentAndMask(alignment));
	if ((uintptr_t)(allocation.data) % alignment != 0) {
		virtualStack_pop();
		return false;
	}
	for (int i = 0; i < paddedSize; i++) {
		allocation.data[i] = i * stride + start;
	}
	if (maxDepth > 1) {
		if (!bruteTest(maxSize, maxDepth - 1)) {
			virtualStack_pop();
			return false;
		}
	}
	for (int i = 0; i < paddedSize; i++) {
		if (allocation.data[i] != (uint8_t)(i * stride + start)) {
			virtualStack_pop();
			return false;
		}
	}
	virtualStack_pop();
	return true;
}

START_TEST(VirtualStack)
	{ // Single threaded
		// TODO: Allocate structures with explicit alignment requirements exceeding the largest element's size.
		VirtualStackAllocation<int32_t> x(9);
		{
			ASSERT_EQUAL((uintptr_t)(x.getUnsafe()) % alignof(int32_t), 0);
			x[0] = 01;
			x[1] = 12;
			x[2] = 23;
			x[3] = 34;
			x[4] = 45;
			x[5] = 56;
			x[6] = 67;
			x[7] = 78;
			x[8] = 89;
			SafePointer<int32_t> pointerY;
			{
				VirtualStackAllocation<int32_t> y(3);
				pointerY = y;
				// Check that the memory address pointed to is evenly divisible by the type's alignment.
				ASSERT_EQUAL((uintptr_t)(y.getUnsafe()) % alignof(int32_t), 0);
				y[0] =  2147483000;
				y[1] = -2147483000;
				y[2] =  65;
				#ifdef SAFE_POINTER_CHECKS
					// This should crash because -1 is outside of the 0..2 range.
					ASSERT_CRASH(y[-1]);
				#endif
				// Reading within bounds and checking that the data was stored correctly.
				ASSERT_EQUAL(y[0],  2147483000);
				ASSERT_EQUAL(y[1], -2147483000);
				ASSERT_EQUAL(y[2],  65);
				#ifdef SAFE_POINTER_CHECKS
					// This should crash because 3 is outside of the 0..2 range.
					ASSERT_CRASH(y[3]);
				#endif
			}
			#ifdef SAFE_POINTER_CHECKS
				// This should crash because pointerY points to memory that was freed when y's scope ended.
				ASSERT_CRASH(pointerY[0]);
			#endif
		}
		#ifdef SAFE_POINTER_CHECKS
			ASSERT_CRASH(x[-1]);
		#endif
		ASSERT_EQUAL(x[0], 01);
		ASSERT_EQUAL(x[1], 12);
		ASSERT_EQUAL(x[2], 23);
		ASSERT_EQUAL(x[3], 34);
		ASSERT_EQUAL(x[4], 45);
		ASSERT_EQUAL(x[5], 56);
		ASSERT_EQUAL(x[6], 67);
		ASSERT_EQUAL(x[7], 78);
		ASSERT_EQUAL(x[8], 89);
		#ifdef SAFE_POINTER_CHECKS
			ASSERT_CRASH(x[9]);
		#endif
		// TODO: Try to access memory from another thread and assert that it triggers an exception.
	}
	{ // Single threaded bruteforce test
		ASSERT(bruteTest(10000, 1000));
	}
	{ // Multi threaded bruteforce test
		const int jobCount = 10;
		bool results[jobCount] = {};
		List<std::function<void()>> jobs;
		for (int i = 0; i < jobCount; i++) {
			bool* result = &results[i];
			jobs.push([result, i](){
				*result = bruteTest(10000, 1000);
			});
		}
		threadedWorkFromList(jobs);
		for (int i = 0; i < jobCount; i++) {
			ASSERT(results[i]);
		}
	}
END_TEST
