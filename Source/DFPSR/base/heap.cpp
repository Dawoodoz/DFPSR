// zlib open source license
//
// Copyright (c) 2024 David Forsgren Piuva
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

#include "heap.h"
#include "../api/stringAPI.h"
#include <mutex>
#include <thread>
#include <stdio.h>
#include <new>

// Get settings from here.
#include "../settings.h"

// Enable this flag to print the number of used heap allocations with calls to heap_allocate and heap_free.
//   This allow detecting memory leaks by confirming that it reaches zero when the application terminates.
//#define DSR_PRINT_ALLOCATION_COUNT

namespace dsr {
	// The framework's maximum memory alignment is either the largest float SIMD vector or the thread safe alignment.
	static const uintptr_t heapAlignment = DSR_MAXIMUM_ALIGNMENT;
	static const uintptr_t heapAlignmentAndMask = memory_createAlignmentAndMask(heapAlignment);

	// Because locking is recursive, it is safest to just have one global mutex for allocating, freeing and manipulating use counters.
	//   Otherwise each use counter would need to store the thread identity and recursive depth for each heap allocation.
	static thread_local intptr_t lockDepth = 0;
	std::mutex memoryLock;

	// Calculates the largest power of two allocation size that does not overflow a pointer on the target platform.
	constexpr int calculateBinCount() {
		intptr_t p = 0;
		while (true) {
			uintptr_t result = ((uintptr_t)1 << p) * heapAlignment;
			// Once the value overflows in uintptr_t we have found the index that can not be used, which is also the bin count when including index zero.
			if (result == 0) {
				return p;
			}
			p++;
		}
	}
	static const int MAX_BIN_COUNT = calculateBinCount();

	static int32_t getBinIndex(uintptr_t minimumSize) {
		for (intptr_t p = 0; p < MAX_BIN_COUNT; p++) {
			uintptr_t result = ((uintptr_t)1 << p) * heapAlignment;
			if (result >= minimumSize) {
				return p;
			}
		}
		// Failed to match any bin.
		return -1;
	}

	static const uint8_t heapFlag_recycled = 1 << 0;
	struct HeapHeader : public AllocationHeader {
		HeapHeader *nextRecycled = nullptr;
		HeapDestructor destructor = nullptr;
		uintptr_t useCount = 0; // How many handles that point to the data.
		// TODO: Reset to zero when allocating and let the caller place additional information here.
		//void *userPtrA = nullptr;
		//void *userPtrB = nullptr;
		//void *userPtrC = nullptr;
		//uint32_t userData = 0;
		uint8_t flags = 0; // Flags use the heapFlag_ prefix.
		uint8_t binIndex; // Recycling bin index to use when freeing the allocation.
		HeapHeader(uintptr_t totalSize, uint8_t binIndex)
		: AllocationHeader(totalSize, false), binIndex(binIndex) {}
	};
	static const uintptr_t heapHeaderPaddedSize = memory_getPaddedSize(sizeof(HeapHeader), heapAlignment);

	AllocationHeader *heap_getHeader(uint8_t const * const allocation) {
		return (AllocationHeader*)(allocation - heapHeaderPaddedSize);
	}

	inline HeapHeader *headerFromAllocation(uint8_t const * const allocation) {
		return (HeapHeader*)(allocation - heapHeaderPaddedSize);
	}

	inline uint8_t *allocationFromHeader(HeapHeader const * const header) {
		return (uint8_t*)header + heapHeaderPaddedSize;
	}

	void heap_increaseUseCount(uint8_t const * const allocation) {
		HeapHeader *header = headerFromAllocation(allocation);
		if (lockDepth == 0) memoryLock.lock();
		header->useCount++;
		if (lockDepth == 0) memoryLock.unlock();
	}

	void heap_decreaseUseCount(uint8_t const * const allocation) {
		HeapHeader *header = headerFromAllocation(allocation);
		if (lockDepth == 0) memoryLock.lock();
		if (header->useCount == 0) {
			printf("Heap error: Decreasing a count that is already zero!\n");
		} else {
			header->useCount--;
			if (header->useCount == 0) {
				lockDepth++;
				heap_free((uint8_t*)allocation);
				lockDepth--;
			}
		}
		if (lockDepth == 0) memoryLock.unlock();
	}

	uintptr_t heap_getUseCount(uint8_t const * const allocation) {
		return headerFromAllocation(allocation)->useCount;
	}

	uint64_t heap_getAllocationSize(uint8_t const * const allocation) {
		return headerFromAllocation(allocation)->totalSize - heapHeaderPaddedSize;
	}

	// A block of memory where heap data can be allocated.
	struct HeapMemory {
		HeapMemory *prevHeap = nullptr;
		uint8_t *top = nullptr; // The start of the arena, where the allocation pointer is when full.
		uint8_t *allocationPointer = nullptr; // The allocation pointer that moves from bottom to top when filling the arena.
		uint8_t *bottom = nullptr; // The end of the arena, where the allocation pointer is when empty.
		HeapMemory(uintptr_t size) {
			this->top = (uint8_t*)(operator new (size));
			this->bottom = this->top + size;
			this->allocationPointer = this->bottom;
		}
		~HeapMemory() {
			if (this->top != nullptr) {
				delete this->top;
				this->top = nullptr;
			}
			this->allocationPointer = nullptr;
			this->bottom = nullptr;
		}
	};

	// The total number of used heap allocations, excluding recycled memory.
	// Only accessed when defaultHeap.poolLock is locked.
	static intptr_t allocationCount = 0;

	// The heap can have memory freed after its own destruction by telling the remaining allocations to clean up after themselves.
	struct HeapPool {
		HeapMemory *lastHeap = nullptr;
		HeapHeader *recyclingBin[MAX_BIN_COUNT] = {};
		bool terminating = false;
		void cleanUp() {
			// If memory safety checks are enabled, then we should indicate that everything is fine with the memory once cleaning up.
			//   There is however no way to distinguish between leaking memory and not yet having terminated everything, so there is no leak warning to print.
			#ifdef SAFE_POINTER_CHECKS
				printf("All heap memory was freed without leaks.\n");
			#endif
			HeapMemory *nextHeap = this->lastHeap;
			while (nextHeap != nullptr) {
				HeapMemory *currentHeap = nextHeap;
				nextHeap = currentHeap->prevHeap;
				operator delete(currentHeap);
			}
			this->lastHeap = nullptr;
		}
		HeapPool() {}
		~HeapPool() {
			memoryLock.lock();
				this->terminating = true;
				if (allocationCount == 0) {
					this->cleanUp();
				}
			memoryLock.unlock();
		}
	};

	static UnsafeAllocation tryToAllocate(HeapMemory &heap, uintptr_t paddedSize, uintptr_t alignmentAndMask, uint8_t binIndex) {
		UnsafeAllocation result(nullptr, nullptr);
			uint8_t *dataPointer = (uint8_t*)(((uintptr_t)(heap.allocationPointer) - paddedSize) & alignmentAndMask);
			AllocationHeader *headerPointer = (AllocationHeader*)(dataPointer - heapHeaderPaddedSize);
			if ((uint8_t*)headerPointer >= heap.top) {
				// There is enough space, so confirm the allocation.
				result = UnsafeAllocation(dataPointer, headerPointer);
				// Write data to the header.
				*headerPointer = HeapHeader((uintptr_t)heap.allocationPointer - (uintptr_t)headerPointer, binIndex);
				// Reserve the data in the heap by moving the allocation pointer.
				heap.allocationPointer = (uint8_t*)headerPointer;
			}
		return result;
	}

	static UnsafeAllocation tryToAllocate(HeapPool &pool, uintptr_t paddedSize, uintptr_t alignmentAndMask, uint8_t binIndex) {
		// Start with the most recent heap, which is most likely to have enough space.
		HeapMemory *currentHeap = pool.lastHeap;
		while (currentHeap != nullptr) {
			UnsafeAllocation result = tryToAllocate(*currentHeap, paddedSize, heapAlignmentAndMask, binIndex);
			if (result.data != nullptr) {
				return result;
			}
			currentHeap = currentHeap->prevHeap;
		}
		// If no free space could be found, allocate a new memory block.
		uintptr_t allocationSize = 16777216;
		uintptr_t usefulSize = paddedSize << 4; // TODO: Handle integer overflow.
		if (usefulSize > allocationSize) allocationSize = usefulSize;
		// Append to the linked list of memory.
		HeapMemory *previousHeap = pool.lastHeap;
		pool.lastHeap = new HeapMemory(allocationSize);
		pool.lastHeap->prevHeap = previousHeap;
		// Make one last attempt at allocating the memory.
		return tryToAllocate(*(pool.lastHeap), paddedSize, heapAlignmentAndMask, binIndex);
	}

	static HeapPool defaultHeap;

	#ifdef DSR_PRINT_ALLOCATION_COUNT
		inline void printAllocationCount() {
			printf("Allocation count = %ld\n", allocationCount);
		}
	#else
		inline void printAllocationCount() {}
	#endif

	UnsafeAllocation heap_allocate(uintptr_t minimumSize, bool zeroed) {
		int32_t binIndex = getBinIndex(minimumSize);
		UnsafeAllocation result(nullptr, nullptr);
		if (binIndex == -1) {
			// If the requested allocation is so big that there is no power of two that can contain it without overflowing the address space, then it can not be allocated.
			printf("Heap error: Exceeded the maximum size when trying to allocate!\n");
		} else {
			uintptr_t paddedSize = ((uintptr_t)1 << binIndex) * heapAlignment;
			if (lockDepth == 0) memoryLock.lock();
			allocationCount++;
			printAllocationCount();
			// Look for pre-existing allocations in the recycling bins.
			HeapHeader *binHeader = defaultHeap.recyclingBin[binIndex];
			if (binHeader != nullptr) {
				// Make the recycled allocation's tail into the new head.
				defaultHeap.recyclingBin[binIndex] = binHeader->nextRecycled;
				// Mark the allocation as not recycled. (assume that it was recycled when found in the bin)
				binHeader->flags &= ~heapFlag_recycled;
				result = UnsafeAllocation(allocationFromHeader(binHeader), binHeader);
			} else {
				// Look for a heap with enough space for a new allocation.
				result = tryToAllocate(defaultHeap, paddedSize, heapAlignmentAndMask, binIndex);
				if (result.data == nullptr) {
					printf("Heap error: Failed to allocate more memory!\n");
				}
			}
			if (lockDepth == 0) memoryLock.unlock();
			if (zeroed && result.data != nullptr) {
				memset(result.data, 0, paddedSize);
			}
		}
		return result;
	}

	void heap_setAllocationDestructor(uint8_t const * const allocation, HeapDestructor destructor) {
		HeapHeader *header = headerFromAllocation(allocation);
		header->destructor = destructor;
	}

	void heap_free(uint8_t * const allocation) {
		// TODO: The mutex hangs when calling destructors recursively.
		if (lockDepth == 0) memoryLock.lock();
		// Get the recycled allocation's header.
		HeapHeader *header = headerFromAllocation(allocation);
		if (header->flags & heapFlag_recycled) {
			printf("Heap error: A heap allocation was freed twice!\n");
		} else {
			// Check if a destructor needs to be called.
			if (header->destructor != nullptr) {
				// Call the destructor without using the mutex (lockDepth > 0).
				lockDepth++;
				header->destructor(allocation);
				lockDepth--;
				assert(lockDepth >= 0);
				// Remove the pointer to the destructor so that it is not called again for the next allocation.
				header->destructor = nullptr;
			}
			int binIndex = header->binIndex;
			if (binIndex >= MAX_BIN_COUNT) {
				printf("Heap error: Out of bound recycling bin index in corrupted head of freed allocation!\n");
			} else {
				// Make any previous head from the bin into the new tail.
				HeapHeader *oldHeader = defaultHeap.recyclingBin[binIndex];
				header->nextRecycled = oldHeader;
				// Mark the allocation as recycled.
				header->flags |= heapFlag_recycled;
				// Store the newly recycled allocation in the bin.
				defaultHeap.recyclingBin[binIndex] = header;
				header->nextRecycled = oldHeader;
			}
		}
		// By decreasing the count after recursive calls to destructors, we can make sure that the arena is freed last.
		// If a destructor allocates new memory, it will have to allocate a new arena and then clean it up again.
		allocationCount--;
		printAllocationCount();
		// If the heap has been told to terminate and we reached zero allocations, we can tell it to clean up.
		if (defaultHeap.terminating && allocationCount == 0) {
			defaultHeap.cleanUp();
		}
		if (lockDepth == 0) memoryLock.unlock();
	}

	void impl_throwAllocationFailure() {
		throwError(U"Failed to allocate memory for a new object!\n");
	}

	void impl_throwNullException() {
		throwError(U"Null handle exception!\n");
	}
}
