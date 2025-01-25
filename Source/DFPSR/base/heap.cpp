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

#include "heap.h"
#include "../api/stringAPI.h"
#include <mutex>
#include <thread>
#include <stdio.h>
#include <new>

// Get settings from here.
#include "../settings.h"

namespace dsr {
	using HeapFlag = uint16_t;
	using BinIndex = uint16_t;

	// The framework's maximum memory alignment is either the largest float SIMD vector or the thread safe alignment.
	static const uintptr_t heapAlignment = DSR_MAXIMUM_ALIGNMENT;
	static const uintptr_t heapAlignmentAndMask = memory_createAlignmentAndMask(heapAlignment);

	// Because locking is recursive, it is safest to just have one global mutex for allocating, freeing and manipulating use counters.
	//   Otherwise each use counter would need to store the thread identity and recursive depth for each heap allocation.
	static thread_local intptr_t lockDepth = 0;
	std::mutex memoryLock;

	// The free function is hidden, because all allocations are forced to use reference counting,
	//   so that a hard exit can disable recursive calls to heap_free by incrementing all reference counters.
	static void heap_free(void * const allocation);

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
	// TODO: Leave a few bins empty in the beginning until reaching heapAlignment from a minimum alignment.
	static const int LOWEST_BIN_INDEX = 0;
	static const int MAX_BIN_COUNT = calculateBinCount();

	inline uintptr_t getBinSize(BinIndex index) {
		return (((uintptr_t)1) << ((uintptr_t)index)) * heapAlignment;
	}

	static BinIndex getBinIndex(uintptr_t minimumSize) {
		for (intptr_t p = 0; p < MAX_BIN_COUNT; p++) {
			uintptr_t result = getBinSize(p);
			if (result >= minimumSize) {
				//printf("getBinIndex %i from %i\n", (int)p, (int)minimumSize);
				return p;
			}
		}
		// Failed to match any bin.
		return -1;
	}

	static const HeapFlag heapFlag_recycled = 1 << 0;
	struct HeapHeader : public AllocationHeader {
		// Because nextRecycled and usedSize have mutually exclusive lifetimes, they can share memory location.
		union {
			// When allocated
			uintptr_t usedSize; // The actual size requested.
			// When recycled
			HeapHeader *nextRecycled = nullptr;
		};
		HeapDestructor destructor;
		uintptr_t useCount = 0; // How many handles that point to the data.
		// TODO: Allow placing a pointer to a singleton in another heap allocation, which will simply be freed using heap_free when the owner is freed.
		//       This allow accessing any amount of additional information shared between all handles to the same payload.
		//       Useful when you in hindsight realize that you need more information attached to something that is already created and shared, like a value allocated image needing a texture.
		//       Check if it already exists. If it does not exist, lock the allocation mutex and check again if it still does not exist, before allocating the singleton.
		//         Then there will only be a mutex overhead for accessing the singleton when accessed for the first time.
		//         Once created, it can not go away until the allocation that knows about it is gone.
		//       If using the reference counting, the singleton could also be reused between different allocations.
		// void *singleton = nullptr;
		// TODO: Allow the caller to access custom bit flags in allocations.
		// uint32_t userFlags = 0;
		HeapFlag flags = 0; // Flags use the heapFlag_ prefix.
		BinIndex binIndex = 0; // Recycling bin index to use when freeing the allocation.
		HeapHeader(uintptr_t totalSize)
		: AllocationHeader(totalSize, false, "Nameless heap allocation") {}
		inline uintptr_t getAllocationSize() {
			return getBinSize(this->binIndex);
		}
		inline uintptr_t getUsedSize() {
			if (this->isRecycled()) {
				return 0;
			} else {
				return this->usedSize;
			}
		}
		inline uintptr_t setUsedSize(uintptr_t size) {
			//printf("setUsedSize: try %i\n", (int)size);
			//printf("  MAX_BIN_COUNT: %i\n", (int)MAX_BIN_COUNT);
			if (!(this->isRecycled())) {
				uintptr_t allocationSize = getAllocationSize();
				//printf("  binIndex: %i\n", (int)this->binIndex);
				//printf("  allocationSize: %i\n", (int)allocationSize);
				if (size > allocationSize) {
					//printf("  too big!\n");
					size = allocationSize;
				}
				this->usedSize = size;
				//printf("  assigned size: %i\n", (int)this->usedSize);
				return size;
			} else {
				return 0;
			}
		}
		inline bool isRecycled() const {
			return (this->flags & heapFlag_recycled) != 0;
		}
		inline void makeRecycled() {
			this->flags |= heapFlag_recycled;
		}
		inline void makeUsed() {
			this->flags &= ~heapFlag_recycled;
		}
	};
	static const uintptr_t heapHeaderPaddedSize = memory_getPaddedSize(sizeof(HeapHeader), heapAlignment);

	AllocationHeader *heap_getHeader(void * const allocation) {
		return (AllocationHeader*)((uint8_t*)allocation - heapHeaderPaddedSize);
	}

	inline HeapHeader *headerFromAllocation(void const * const allocation) {
		return (HeapHeader *)((uint8_t*)allocation - heapHeaderPaddedSize);
	}

	inline void *allocationFromHeader(HeapHeader * const header) {
		return ((uint8_t*)header) + heapHeaderPaddedSize;
	}
	inline void const *allocationFromHeader(HeapHeader const * const header) {
		return ((uint8_t const *)header) + heapHeaderPaddedSize;
	}

	#ifdef SAFE_POINTER_CHECKS
		void heap_setAllocationName(void * const allocation, const char *name) {
			if (allocation != nullptr) {
				HeapHeader *header = headerFromAllocation(allocation);
				header->name = name;
			}
		}

		const char * heap_getAllocationName(void * const allocation) {
			if (allocation == nullptr) {
				return "none";
			} else {
				HeapHeader *header = headerFromAllocation(allocation);
				return header->name;
			}
		}

		uintptr_t heap_getPaddedSize(void const * const allocation) {
			if (allocation == nullptr) {
				return 0;
			} else {
				HeapHeader *header = headerFromAllocation(allocation);
				return memory_getPaddedSize_usingAndMask(header->getUsedSize(), heapAlignmentAndMask);
			}
		}
	#endif

	uintptr_t heap_getAllocationSize(void const * const allocation) {
		HeapHeader *header = headerFromAllocation(allocation);
		return getBinSize(header->binIndex);
	}

	uintptr_t heap_getUsedSize(void const * const allocation) {
		uintptr_t result = 0;
		if (allocation != nullptr) {
			HeapHeader *header = headerFromAllocation(allocation);
			result = header->getUsedSize();
			//printf("  heap_getUsedSize: get %i\n", (int)result);
		}
		return result;
	}

	uintptr_t heap_setUsedSize(void * const allocation, uintptr_t size) {
		uintptr_t result = 0;
		if (allocation != nullptr) {
			//uintptr_t allocationSize = heap_getAllocationSize(allocation);
			HeapHeader *header = headerFromAllocation(allocation);
			result = header->setUsedSize(size);
			//printf("  heap_setUsedSize: try %i get %i\n", (int)size, (int)result);
		}
		return result;
	}

	void heap_increaseUseCount(void const * const allocation) {
		if (allocation != nullptr) {
			HeapHeader *header = headerFromAllocation(allocation);
			if (lockDepth == 0) memoryLock.lock();
			//printf("heap_increaseUseCount called for allocation @ %ld\n", (uintptr_t)allocation);
			//printf("    Use count: %ld -> %ld\n", header->useCount, header->useCount + 1);
			//#ifdef SAFE_POINTER_CHECKS
			//	printf("    ID: %lu\n", header->allocationIdentity);
			//	printf("    Name: %s\n", header->name ? header->name : "<nameless>");
			//#endif
			header->useCount++;
			if (lockDepth == 0) memoryLock.unlock();
		}
	}

	void heap_decreaseUseCount(void const * const allocation) {
		if (allocation != nullptr) {
			HeapHeader *header = headerFromAllocation(allocation);
			if (lockDepth == 0) memoryLock.lock();
			//printf("heap_decreaseUseCount called for allocation @ %ld\n", (uintptr_t)allocation);
			//printf("    Use count: %ld -> %ld\n", header->useCount, header->useCount - 1);
			//#ifdef SAFE_POINTER_CHECKS
			//	printf("    ID: %lu\n", header->allocationIdentity);
			//	printf("    Name: %s\n", header->name ? header->name : "<nameless>");
			//#endif
			if (header->useCount == 0) {
				throwError(U"Heap error: Decreasing a count that is already zero!\n");
			} else {
				header->useCount--;
				if (header->useCount == 0) {
					lockDepth++;
					heap_free((void*)allocation);
					lockDepth--;
				}
			}
			if (lockDepth == 0) memoryLock.unlock();
		}
	}

	uintptr_t heap_getUseCount(void const * const allocation) {
		return headerFromAllocation(allocation)->useCount;
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
				// Can't allocate more memory after freeing all memory.
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

	static UnsafeAllocation tryToAllocate(HeapMemory &heap, uintptr_t paddedSize, uintptr_t alignmentAndMask) {
		UnsafeAllocation result(nullptr, nullptr);
			uint8_t *dataPointer = (uint8_t*)(((uintptr_t)(heap.allocationPointer) - paddedSize) & alignmentAndMask);
			AllocationHeader *headerPointer = (AllocationHeader*)(dataPointer - heapHeaderPaddedSize);
			if ((uint8_t*)headerPointer >= heap.top) {
				// There is enough space, so confirm the allocation.
				result = UnsafeAllocation(dataPointer, headerPointer);
				// Write data to the header.
				*headerPointer = HeapHeader((uintptr_t)heap.allocationPointer - (uintptr_t)headerPointer);
				// Reserve the data in the heap by moving the allocation pointer.
				heap.allocationPointer = (uint8_t*)headerPointer;
			}
		return result;
	}

	static UnsafeAllocation tryToAllocate(HeapPool &pool, uintptr_t paddedSize, uintptr_t alignmentAndMask) {
		// Start with the most recent heap, which is most likely to have enough space.
		HeapMemory *currentHeap = pool.lastHeap;
		while (currentHeap != nullptr) {
			UnsafeAllocation result = tryToAllocate(*currentHeap, paddedSize, heapAlignmentAndMask);
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
		return tryToAllocate(*(pool.lastHeap), paddedSize, heapAlignmentAndMask);
	}

	static HeapPool defaultHeap;

	UnsafeAllocation heap_allocate(uintptr_t minimumSize, bool zeroed) {
		UnsafeAllocation result(nullptr, nullptr);
		int32_t binIndex = getBinIndex(minimumSize);
		//printf("heap_allocate\n");
		//printf("  minimumSize: %i\n", (int)minimumSize);
		//printf("  binIndex: %i\n", (int)binIndex);
		if (binIndex == -1) {
			// If the requested allocation is so big that there is no power of two that can contain it without overflowing the address space, then it can not be allocated.
			throwError(U"Heap error: Exceeded the maximum size when trying to allocate!\n");
		} else {
			uintptr_t paddedSize = ((uintptr_t)1 << binIndex) * heapAlignment;
			if (lockDepth == 0) memoryLock.lock();
			allocationCount++;
			// Look for pre-existing allocations in the recycling bins.
			HeapHeader *binHeader = defaultHeap.recyclingBin[binIndex];
			if (binHeader != nullptr) {
				// Make the recycled allocation's tail into the new head.
				defaultHeap.recyclingBin[binIndex] = binHeader->nextRecycled;
				// Clear the pointer to make room for the allocation's size in the union.
				binHeader->nextRecycled = nullptr;
				// Mark the allocation as not recycled. (assume that it was recycled when found in the bin)
				binHeader->makeUsed();
				binHeader->reuse(false, "Nameless reused allocation");
				result = UnsafeAllocation((uint8_t*)allocationFromHeader(binHeader), binHeader);
			} else {
				// Look for a heap with enough space for a new allocation.
				result = tryToAllocate(defaultHeap, paddedSize, heapAlignmentAndMask);
				if (result.data == nullptr) {
					throwError(U"Heap error: Failed to allocate more memory!\n");
				}
			}
			if (lockDepth == 0) memoryLock.unlock();
			if (zeroed && result.data != nullptr) {
				memset(result.data, 0, paddedSize);
			}
		}
		if (result.data != nullptr) {
			// Get the header.
			HeapHeader *head = (HeapHeader*)(result.header);
			// Tell the allocation where it should be recycled when freed.
			head->binIndex = binIndex;
			// Tell the allocation how many of the bytes that are used.
			head->setUsedSize(minimumSize);
			// Give a debug name to the allocation if we are debugging.
			//printf("Allocated memory @ %ld\n", (uintptr_t)result.data);
			//printf("    Use count = %ld\n", head->useCount);
			//#ifdef SAFE_POINTER_CHECKS
			//	printf("    ID = %lu\n", head->allocationIdentity);
			//#endif
		}
		return result;
	}

	void heap_setAllocationDestructor(void * const allocation, const HeapDestructor &destructor) {
		HeapHeader *header = headerFromAllocation(allocation);		
		header->destructor = destructor;
	}

	static void heap_free(void * const allocation) {
		if (lockDepth == 0) memoryLock.lock();
		// Get the recycled allocation's header.
		HeapHeader *header = headerFromAllocation(allocation);
		if (header->isRecycled()) {
			throwError(U"Heap error: A heap allocation was freed twice!\n");
		} else {
			// Call the destructor without using the mutex (lockDepth > 0).
			lockDepth++;
			//printf("heap_free called for allocation @ %ld\n", (uintptr_t)allocation);
			//printf("    Use count: %ld\n", header->useCount);
			//#ifdef SAFE_POINTER_CHECKS
			//	printf("    ID: %lu\n", header->allocationIdentity);
			//	printf("    Name: %s\n", header->name ? header->name : "<nameless>");
			//#endif
			//printf("    Calling destructor\n");
			// Call the destructor provided with any external resource that also needs to be freed.
			if (header->destructor.destructor) {
				header->destructor.destructor(allocation, header->destructor.externalResource);
			}
			//printf("    Finished destructor\n");
			lockDepth--;
			assert(lockDepth >= 0);
			// Remove the destructor so that it is not called again for the next allocation.
			header->destructor = HeapDestructor();
			int binIndex = header->binIndex;
			if (binIndex >= MAX_BIN_COUNT) {
				throwError(U"Heap error: Out of bound recycling bin index in corrupted head of freed allocation!\n");
			} else {
				// Make any previous head from the bin into the new tail.
				HeapHeader *oldHeader = defaultHeap.recyclingBin[binIndex];
				header->nextRecycled = oldHeader;
				// Mark the allocation as recycled.
				header->makeRecycled();
				#ifdef SAFE_POINTER_CHECKS
					// Remove the allocation identity, so that use of freed memory can be detected in SafePointer and Handle.
					header->allocationIdentity = 0;
					header->threadHash = 0;
				#endif
				// Store the newly recycled allocation in the bin.
				defaultHeap.recyclingBin[binIndex] = header;
				header->nextRecycled = oldHeader;
			}
		}
		// By decreasing the count after recursive calls to destructors, we can make sure that the arena is freed last.
		// If a destructor allocates new memory, it will have to allocate a new arena and then clean it up again.
		allocationCount--;
		// If the heap has been told to terminate and we reached zero allocations, we can tell it to clean up.
		if (defaultHeap.terminating && allocationCount == 0) {
			defaultHeap.cleanUp();
		}
		if (lockDepth == 0) memoryLock.unlock();
	}

	static void forAllHeapAllocations(HeapMemory &heap, std::function<void(AllocationHeader * header, void * allocation)> callback) {
		uint8_t * current = heap.allocationPointer;
		while (current < heap.bottom) {
			HeapHeader *header = (HeapHeader*)current;
			void *payload = allocationFromHeader(header);
			if (!(header->isRecycled())) {
				callback(header, payload);
			}
			current += header->totalSize;
		}
	}

	void heap_forAllHeapAllocations(std::function<void(AllocationHeader * header, void * allocation)> callback) {
		HeapMemory *currentHeap = defaultHeap.lastHeap;
		while (currentHeap != nullptr) {
			forAllHeapAllocations(*currentHeap, callback);
			currentHeap = currentHeap->prevHeap;
		}
	}

	void heap_hardExitCleaning() {
		// TODO:
		// * Implement a function that iterates over all heap allocations.
		// * Increment the use count for each allocation, to prevent recursive freeing of resources.
		// * Call the destructor on each allocation without freeing any memory, while all memory is still available.
		// Then the arenas can safely be deallocated without looking at individual allocations again.
		allocationCount = 0;
		defaultHeap.terminating = true;
		defaultHeap.cleanUp();
	}

	void impl_throwAllocationFailure() {
		throwError(U"Failed to allocate memory for a new object!\n");
	}

	void impl_throwNullException() {
		throwError(U"Null handle exception!\n");
	}

	void impl_throwIdentityMismatch(uint64_t allocationIdentity, uint64_t pointerIdentity) {
		throwError(U"Identity mismatch! The allocation pointed to had identity ", allocationIdentity, U" but ", pointerIdentity, U" was expected by the pointer from when it was allocated.\n");
	}
}
