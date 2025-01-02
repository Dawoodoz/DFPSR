// zlib open source license
//
// Copyright (c) 2024 to 2025 David Forsgren Piuva
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
//    2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 
//    3. This notice may not be removed or altered from any source
//    distribution.

// An arena memory allocator with recycling bins to provide heap memory.
//   All allocations are aligned to DSR_MAXIMUM_ALIGNMENT to prevent false sharing of cache lines between threads.
//   The space in front of each allocation contains a HeapHeader including:
//     * The total size of the allocation including padding and the header.
//     * How many of the allocated bytes that are actually used.
//     * A reference counter.
//     * A destructor to allow freeing the memory no matter where the reference counter decreases to zero.
//     * A bin index for fast recycling of memory.
//     * Bit flags for keeping track of the allocation's state.
//   In debug mode, the header also contains:
//     * A thread hash to keep track of data ownership.
//       All heap allocations are currently shared among all threads unlike virtual stack memory.
//     * An allocation identity that is unique for each new allocation.
//       When freed, the header's allocation identity is set to zero to prevent accidental use of freed memory.
//       When recycled as a new allocation, the same address gets a new identity, which invalidates any old SafePointer to the same address.
//     * An ascii name to make memory debugging easier.

#ifndef DFPSR_HEAP
#define DFPSR_HEAP

#include "SafePointer.h"

namespace dsr {
	#ifdef SAFE_POINTER_CHECKS
		// Assign a debug name to the allocation.
		//   Only assign constant ascii string literals.
		void heap_setAllocationName(void * const allocation, const char *name);
		// Get the ascii name of allocation, or "none" if allocation is null.
		const char * heap_getAllocationName(void * const allocation);
	#endif

	// Allocate memory in the heap.
	//   The minimumSize argument is the minimum number of bytes to allocate, but the result may give you more than you asked for.
	//   When zeroed is true, the new memory will be zeroed. Otherwise it may contain uninitialized data.
	// Post-condition: Returns pointers to the payload and header.
	UnsafeAllocation heap_allocate(uintptr_t minimumSize, bool zeroed = true);
	inline UnsafeAllocation heap_allocate(uintptr_t minimumSize, bool zeroed, const char *name) {
		UnsafeAllocation result = heap_allocate(minimumSize, zeroed);
		#ifdef SAFE_POINTER_CHECKS
			heap_setAllocationName(result.data, name);
		#endif
		return result;
	}

	// Increase the use count of an allocation.
	//   Does nothing if the allocation is nullptr.
	void heap_increaseUseCount(void const * const allocation);

	// Decrease the use count of an allocation and recycle it when reaching zero.
	//   Does nothing if the allocation is nullptr.
	void heap_decreaseUseCount(void const * const allocation);
	
	// Pre-condition:
	//   allocation points to memory allocated as heap_allocate(...).data because this feature is specific to this allocator.
	// Post-condition:
	//   Returns the number of bytes in the allocation that are actually used, which is used for tight bound checks and knowing how large a buffer is.
	//   Returns 0 if allocation is nullptr.
	uintptr_t heap_getSize(void const * const allocation);

	// Side-effect:
	//   Assigns a new used size to allocation.
	//   Has no effect if allocation is nullptr.
	// Pre-condition:
	//   You may not reserve more memory than what is available in total.
	//   size <= heap_getAllocationSize(allocation)
	void heap_setSize(void * const allocation, uintptr_t size);

	// A function pointer for destructors.
	using HeapDestructorPointer = void(*)(void *toDestroy, void *externalResource);
	struct HeapDestructor {
		// A function pointer to a method taking toDestroy and externalResource as arguments.
		HeapDestructorPointer destructor = nullptr;
		// A pointer for freeing external resources owning the allocation.
		void *externalResource = nullptr;
		HeapDestructor(HeapDestructorPointer destructor = nullptr, void *externalResource = nullptr)
		: destructor(destructor), externalResource(externalResource) {}
		inline void call(void *allocation, void *externalResource) {
			if (this->destructor) {
				this->destructor(allocation, externalResource);
			}
		}
	};

	// Register a destructor function pointer to be called automatically when the allocation is freed.
	//   externalResource is the second argument that will be given to destructor together with the freed memory to destruct.
	void heap_setAllocationDestructor(void * const allocation, const HeapDestructor &destructor);

	// Get the use count outside of transactions without locking.
	uintptr_t heap_getUseCount(void const * const allocation);

	// Pre-condition: The allocation pointer must point to the start of a payload allocated using heap_allocate, no offsets nor other allocators allowed.
	// Post-condition: Returns the number of available bytes in the allocation.
	//                 You may not read a single byte outside of it, because it might include padding that ends at uneven addresses.
	//                 To use more memory than requested, you must round it down to whole elements.
	//                 If the element's size is a power of two, you can pre-compute a bit mask using memory_createAlignmentAndMask for rounding down.
	uint64_t heap_getAllocationSize(void const * const allocation);

	// Pre-condition: The allocation pointer must point to the start of a payload allocated using heap_allocate, no offsets nor other allocators allowed.
	// Post-condition: Returns a pointer to the heap allocation's header, which is used to construct safe pointers.
	AllocationHeader *heap_getHeader(void * const allocation);

	// Only a pointer is needed, so that it can be sent as a function pointer to X11.
	// TODO: Use the allocation head's alignment as the minimum alignment by combining the masks in compile time.
	//       Then it is possible to place the padded allocation header for heap memory at a fixed offset from the allocation start, so that the head can be accessed.
	//       No extra offsets are allowed on the pointer used to free the memory.
	// TODO: Have a global variable containing the default memory pool.
	//       When it is destructed, all allocations that are empty will be freed and a termination flag will be enabled so that any more allocations being freed after it will free the memory themselves.
	void heap_free(void * const allocation);

	// Helper methods to prevent cyclic dependencies between strings and buffers when handles must be inlined for performance. Do not call these yourself.
	void impl_throwAllocationFailure();
	void impl_throwNullException();
	void impl_throwIdentityMismatch(uint64_t allocationIdentity, uint64_t pointerIdentity);
}

#endif
