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
//   All allocations are reference counted, because the memory allocator itself may increase the reference count as needed.
//     * An allocation with use count 0 will remain until the use count changes and reaches zero again.
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

// Dimensions
//   used size <= padded size <= allocation size
// * Size defines the used bytes that represent something, which affects destruction of elements.
//   Size can change from 0 to allocation size without having to move the data.
// * The padded size defines the region where memory access is allowed for SafePointer.
//   Padded size is computed by rounding up to whole blocks of DSR_MAXIMUM_ALIGNMENT.
// * Allocation size is the available space to work with.
//   To change the allocation's size, one must move to another memory location.

#ifndef DFPSR_HEAP
#define DFPSR_HEAP

#include "SafePointer.h"
#include <functional>

namespace dsr {
	#ifdef SAFE_POINTER_CHECKS
		// Assign a debug name to the allocation.
		//   Does nothing if allocation is nullptr.
		//   Only assign constant ascii string literals.
		void heap_setAllocationName(void const * const allocation, const char *name);
		// Get the ascii name of allocation, or "none" if allocation is nullptr.
		const char * heap_getAllocationName(void const * const allocation);
		// Gets the size padded out to whole blocks of heap alignment, for constructing a SafePointer.
		uintptr_t heap_getPaddedSize(void const * const allocation);
		// Set the serialization function for a certain allocation, so that going through memory allocations will display its content when debugging memory leaks.
		void setAllocationSerialization(void const * const allocation, AllocationSerialization method);
		// Get the function used to interpret the allocation's content.
		AllocationSerialization getAllocationSerialization(void const * const allocation);
	#endif

	// TODO: Allow allocating fixed size allocations using a typename and pre-calculate the bin index in compile time.
	//       This requires the bin sizes to be independent of DSR_MAXIMUM_ALIGNMENT, possibly by leaving a few bins
	//       unused in the beginning and start counting with the header size as the smallest allocation size.
	// Allocate memory in the heap.
	//   The minimumSize argument is the minimum number of bytes to allocate, but the result may give you more than you asked for.
	//   To allow representing empty files using buffers, it is allowed to create an allocation of zero bytes.
	//   When zeroed is true, the new memory will be zeroed. Otherwise it may contain uninitialized data.
	// Post-condition: Returns pointers to the payload and header.
	UnsafeAllocation heap_allocate(uintptr_t minimumSize, bool zeroed = true);

	// Increase the use count of an allocation.
	//   Does nothing if the allocation is nullptr.
	void heap_increaseUseCount(void const * const allocation);
	void heap_increaseUseCount(AllocationHeader const * const header);

	// Decrease the use count of an allocation and recycle it when reaching zero.
	//   Does nothing if the allocation is nullptr.
	void heap_decreaseUseCount(void const * const allocation);
	void heap_decreaseUseCount(AllocationHeader const * const header);

	// Pre-condition:
	//   allocation points to memory allocated as heap_allocate(...).data because this feature is specific to this allocator.
	// Post-condition:
	//   Returns the number of bytes in the allocation that are actually used, which is used for tight bound checks and knowing how large a buffer is.
	//   Returns 0 if allocation is nullptr.
	uintptr_t heap_getUsedSize(void const * const allocation);
	uintptr_t heap_getUsedSize(AllocationHeader const * const header);

	// Side-effect:
	//   Assigns a new used size to allocation.
	//   Has no effect if allocation is nullptr.
	// Pre-condition:
	//   You may not reserve more memory than what is available in total.
	//   size <= heap_getAllocationSize(allocation)
	//   If exceeded, size will be limited by the allocation's size.
	// Post-condition:
	//   Returns the assigned size, which is the given size, an exceeded allocation size, or zero for an allocation that does not exist.
	uintptr_t heap_setUsedSize(void * const allocation, uintptr_t size);
	uintptr_t heap_setUsedSize(AllocationHeader * const header, uintptr_t size);

	// A function pointer for destructors.
	using HeapDestructorPointer = void(*)(void *toDestroy, void *externalResource);
	struct HeapDestructor {
		// A function pointer to a method taking toDestroy and externalResource as arguments.
		HeapDestructorPointer destructor = nullptr;
		// A pointer for freeing external resources owning the allocation.
		void *externalResource = nullptr;
		// Constructor.
		HeapDestructor(HeapDestructorPointer destructor = nullptr, void *externalResource = nullptr)
		: destructor(destructor), externalResource(externalResource) {}
	};

	// Register a destructor function pointer to be called automatically when the allocation is freed.
	//   externalResource is the second argument that will be given to destructor together with the freed memory to destruct.
	void heap_setAllocationDestructor(void * const allocation, const HeapDestructor &destructor);

	// Get the use count outside of transactions without locking.
	uintptr_t heap_getUseCount(void const * const allocation);
	uintptr_t heap_getUseCount(AllocationHeader const * const header);

	// Pre-condition: The allocation pointer must point to the start of a payload allocated using heap_allocate, no offsets nor other allocators allowed.
	// Post-condition: Returns the number of available bytes in the allocation.
	uintptr_t heap_getAllocationSize(void const * const allocation);
	// Pre-condition: The header pointer must point to the allocation head, as returned from heap_allocate or heap_getHeader.
	// Post-condition: Returns the number of available bytes in the allocation.
	uintptr_t heap_getAllocationSize(AllocationHeader const * const header);

	// Get the alignment of the heap, which depends on the largest cache line size.
	uintptr_t heap_getHeapAlignment();

	// Pre-condition: The allocation pointer must point to the start of a payload allocated using heap_allocate, no offsets nor other allocators allowed.
	// Post-condition: Returns a pointer to the heap allocation's header, which is used to construct safe pointers.
	AllocationHeader *heap_getHeader(void * const allocation);

	// Call back for each used heap allocation.
	//   Recycled allocations are not included.
	void heap_forAllHeapAllocations(std::function<void(AllocationHeader * header, void * allocation)> callback);

	// Prints the allocation to the terminal.
	void heap_debugPrintAllocation(void const * const allocation, uintptr_t maxLength = 128u);
	// Prints a list of allocations to the terminal, without creating new memory allocations.
	void heap_debugPrintAllocations(uintptr_t maxLength = 128u);

	// Called by DSR_MAIN_CALLER when the program starts.
	void heap_startingApplication();

	// Called by DSR_MAIN_CALLER when the program closes.
	void heap_terminatingApplication();

	// If terminating the program using std::exit, you can call this first to free all heap memory in the allocator, leaked or not.
	void heap_hardExitCleaning();

	// Used to find the origin of memory leaks in single-threaded tests.
	intptr_t heap_getAllocationCount();

	// Store application defined custom flags, which can be used for debugging memory leaks.
	//   The flags do not take any additional memory, because an allocation head can not allocate less than a whole cache line.
	uint32_t heap_getAllocationCustomFlags(void const * const allocation);
	uint32_t heap_getAllocationCustomFlags(AllocationHeader * header);
	void heap_setAllocationCustomFlags(void const * const allocation, uint32_t customFlags);
	void heap_setAllocationCustomFlags(AllocationHeader * header, uint32_t customFlags);

	// TODO: Allow storing custom information in allocation heads for debugging memory.

	// Helper methods to prevent cyclic dependencies between strings and buffers when handles must be inlined for performance. Do not call these yourself.
	void impl_throwAllocationFailure();
	void impl_throwNullException();
	void impl_throwIdentityMismatch(uint64_t allocationIdentity, uint64_t pointerIdentity);
}

#endif
