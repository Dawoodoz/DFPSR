
#include "../testTools.h"
#include "../../DFPSR/base/virtualStack.h"

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
			VirtualStackAllocation<int32_t> y(3);
			{
				ASSERT_EQUAL((uintptr_t)(y.getUnsafe()) % alignof(int32_t), 0);
				y[0] =  2147483000;
				y[1] = -2147483000;
				y[2] =  65;
				#ifdef SAFE_POINTER_CHECKS
					ASSERT_CRASH(y[-1]);
				#endif
				ASSERT_EQUAL(y[0],  2147483000);
				ASSERT_EQUAL(y[1], -2147483000);
				ASSERT_EQUAL(y[2],  65);
				#ifdef SAFE_POINTER_CHECKS
					ASSERT_CRASH(y[3]);
				#endif
			}
			//#ifdef SAFE_POINTER_CHECKS
			// TODO: Implement and test memory safety against deallocated data by remembering the allocation's identity in the pointer and erasing it when freeing.
			//#endif
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
	}
	{ // Multi threaded
		// TODO: Create a bruteforce test using multiple threads.
	}
END_TEST
