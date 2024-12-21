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
#include <mutex>
#include <thread>
#include "../api/stringAPI.h"

// Get settings from here.
#include "../settings.h"

namespace dsr {
	// The framework's maximum memory alignment is either the largest float SIMD vector or the thread safe alignment.
	static const uintptr_t heapAlignment = DSR_MAXIMUM_ALIGNMENT;
	static const uintptr_t heapAlignmentAndMask = memory_createAlignmentAndMask(heapAlignment);

	static const int maxBinCount = 60;

	static uint8_t powerOfTwoIndex(uint64_t minimumSize) {
		// TODO: Unroll for contant evaluation of limits.
		for (int32_t p = 0; p < 64; p++) {
			uint64_t result = ((uint64_t)1 << p) * heapAlignment;
			if (result >= minimumSize) {
				return p;
			}
		}
		return maxBinCount - 1;
	}

	static uint64_t powerOfTwoSize(uint64_t minimumSize) {
		// TODO: Unroll for contant evaluation of limits.
		for (int32_t p = 0; p < 64; p++) {
			uint64_t result = ((uint64_t)1 << p) * heapAlignment;
			if (result >= minimumSize) {
				return result;
			}
		}
		return ~((uint64_t)0);
	}

	static const uint16_t heapFlag_recycled = 1 << 0;
	struct HeapHeader : public AllocationHeader {
		HeapHeader *nextRecycled = nullptr;
		//uint64_t useCount;
		uint16_t flags = 0;
		uint8_t binIndex;
		HeapHeader(uintptr_t totalSize, uint8_t binIndex)
		: AllocationHeader(totalSize, false), binIndex(binIndex) {}
	};
	static const uintptr_t heapHeaderPaddedSize = memory_getPaddedSize(sizeof(HeapHeader), heapAlignment);

	inline HeapHeader *headerFromAllocation(uint8_t* allocation) {
		return (HeapHeader*)(allocation - heapHeaderPaddedSize);
	}

	inline uint8_t *allocationFromHeader(HeapHeader* header) {
		return (uint8_t*)header + heapHeaderPaddedSize;
	}

	uint64_t heap_getAllocationSize(uint8_t* allocation) {
		return headerFromAllocation(allocation)->totalSize - heapHeaderPaddedSize;
	}

	// A block of memory where heap data can be allocated.
	struct HeapMemory {
		// TODO: Recycle memory using groups of allocations in power of two bytes.
		HeapMemory *prevHeap = nullptr;
		uint8_t *top = nullptr; // The stack pointer is here when completely full.
		uint8_t *allocationPointer = nullptr; // The allocation pointer.
		uint8_t *bottom = nullptr; // The stack pointer is here when empty.
		HeapMemory(uint8_t *top)
		: top(top) {}
		~HeapMemory() {
			if (this->top != nullptr) {
				free(this->top);
				this->top = nullptr;
			}
			this->allocationPointer = nullptr;
			this->bottom = nullptr;
		}
	};

	// The heap can have memory freed after its own destruction by telling the remaining allocations to clean up after themselves.
	struct HeapPool {
		static std::mutex poolLock;
		HeapMemory *lastHeap = nullptr;
		HeapHeader *recyclingBin[65] = {};
		bool terminating = false;
		Heap() {}
		~Heap() {
			this->poolLock.lock();
				this->terminating = true;
				HeapMemory *nextHeap = this->lastHeap;
				while (nextHeap != nullptr) {
					HeapMemory *currentHeap = nextHeap;
					nextHeap = currentHeap->prevHeap;
					delete currentHeap;
				}
			this->poolLock.unlock();
		}
		bool canAccess() {
			bool terminating;
			this->poolLock.lock();
				terminating = this->terminating;
			this->poolLock.unlock();
			return !terminating;
		}
	};

	static UnsafeAllocation tryToAllocate(HeapMemory &heap, uint64_t paddedSize, uintptr_t alignmentAndMask, uint8_t binIndex) {
		UnsafeAllocation result;
			uint8_t *dataPointer = (heap.allocationPointer - paddedSize) & (alignmentAndMask);
			uint8_t *headerPointer -= heapHeaderPaddedSize;
			if (headerPointer >= heap.top) {
				// There is enough space, so confirm the allocation.
				result = UnsafeAllocation(dataPointer, headerPointer);
				*headerPointer = HeapHeader(heap.allocationPointer - headerPointer, binIndex);
				// TODO: Store the size in the head.
			}
		return result;
	}

	// Global mutex
	static Heap defaultHeap;

	UnsafeAllocation heap_allocate(uint64_t minimumSize) {
		// TODO: Get bin index and size at the same time.
		// Round the size up to a pre-defined size for faster re-use of allocations.
		uint64_t paddedSize = powerOfTwoSize(minimumSize);
		uint8_t binIndex = powerOfTwoIndex(minimumSize);
		defaultHeap->poolLock.lock();
		if (!(defaultHeap->terminating)) {
			// Look for pre-existing allocations in the recycling bins.
			HeapHeader *binHeader = defaultHeap.recyclingBin[binIndex];
			if (binHeader != nullptr) {
				// Make the recycled allocation's tail into the new head.
				defaultHeap.recyclingBin[binIndex] = binHeader->nextRecycled;
				// Mark the allocation as not recycled. (assume that it was recycled when found in the bin)
				binHeader->flags &= ~heapFlag_recycled;
				return UnsafeAllocation(allocationFromHeader(binHeader), binHeader);
			}
			// Look for a heap with enough space for a new allocation.
			uint8_t *currentHeap = defaultHeap.lastHeap;
			while (currentHeap != nullptr) {
				UnsafeAllocation allocation = tryToAllocate(*currentHeap, paddedSize, heapAlignmentAndMask, binIndex);
				if (allocation.data != nullptr) {
					return allocation;
				}
				currentHeap = currentHeap.prevHeap;
			}
			// If no free space could be found, allocate a new memory block.
			uint8_t *previousHeap = defaultHeap.lastHeap;
			uint64_t allocationSize = 16777216;
			uint64_t usefulSize = paddedSize << 4; // TODO: Handle integer overflow.
			if (usefulSize > allocationSize) allocationSize = usefulSize;
			defaultHeap.lastHeap = new HeapMemory(malloc(allocationSize));
			defaultHeap.lastHeap->prevHeap
			// TODO: Allocate memory in one of the available bins.
		}
		defaultHeap->poolLock.unlock();
	}

	void heap_free(uint8_t *allocation) {
		defaultHeap->poolLock.lock();
		if (!(defaultHeap->terminating)) {
			HeapHeader *header = allocation - heapHeaderPaddedSize;
			// TODO: Calculate which recycling bin the freed memory belongs to.
			defaultHeap.poolLock.lock();
			// Get the recycled allocation's header and its bin index.
			HeapHeader *newHeader = headerFromAllocation(allocation);
			if (newHeader->flags & heapFlag_recycled) {
				throwError(U"An allocation was freed twice!\n");
				//#ifdef SAFE_POINTER_CHECKS
				// TODO: Print more information when possible.
				//#endif
			} else {
				int binIndex = header->binIndex;
				if (binIndex > 64) {
					throwError(U"Out of bound recycling bin index in corrupted head of freed allocation!\n");
				} else {
					// Make any previous head from the bin into the new tail.
					HeapHeader *oldHeader = defaultHeap.recyclingBin[binIndex];
					newHeader->nextRecycled = oldHeader;
					// Mark the allocation as recycled.
					newHeader->flags |= heapFlag_recycled;
					// Store the newly recycled allocation in the bin.
					defaultHeap.recyclingBin[binIndex] = newHeader;
					// TODO: Place the allocation in the correct recycling bin using a linked list.
					defaultHeap.poolLock.unlock();
				}
			}
		}
		defaultHeap->poolLock.unlock();
	}
}
