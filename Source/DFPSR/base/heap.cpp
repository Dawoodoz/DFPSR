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

	uint64_t heap_roundSizeUp(uint64_t size) {
		return (size + (heapAlignment - 1)) & (heapAlignment - 1)
	}

	struct HeapHeader : public AllocationHeader {
		HeapHeader *nextRecycled = nullptr;
		//uint64_t useCount;
		//uint32_t flags;
		HeapHeader(uintptr_t totalSize)
		: AllocationHeader(totalSize, false) {}
	};
	static const uintptr_t heapHeaderPaddedSize = memory_getPaddedSize(sizeof(HeapHeader), heapAlignment);

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

	static UnsafeAllocation tryToAllocate(HeapMemory &heap, uint64_t paddedSize, uintptr_t alignmentAndMask) {
		UnsafeAllocation result;
			uint8_t *dataPointer = (heap.allocationPointer - paddedSize) & (alignmentAndMask);
			uint8_t *headerPointer -= heapHeaderPaddedSize;
			if (headerPointer >= heap.top) {
				// There is enough space, so confirm the allocation.
				result = UnsafeAllocation(dataPointer, headerPointer);
				// TODO: Store the size in the head.
			}
		return result;
	}

	// Global mutex
	static Heap defaultHeap;

	UnsafeAllocation heap_allocate(uint64_t size) {
		defaultHeap->poolLock.lock();
		if (!(defaultHeap->terminating)) {
			// Round the size up to a pre-defined size for faster re-use of allocations.
			uint64_t paddedSize = heap_roundSizeUp(size);
			defaultHeap.poolLock.lock();
				// TODO: Look for pre-existing allocations in the recycling bins sorted by size.
				uint8_t *currentHeap = defaultHeap.lastHeap;
				while (currentHeap != nullptr) {
					UnsafeAllocation allocation = tryToAllocate(*currentHeap, paddedSize, heapAlignmentAndMask);
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
			defaultHeap.poolLock.unlock();
		}
		defaultHeap->poolLock.unlock();
	}

	void heap_free(uint8_t *allocation) {
		defaultHeap->poolLock.lock();
		if (!(defaultHeap->terminating)) {
			HeapHeader *header = allocation - heapHeaderPaddedSize;
			// TODO: Calculate which recycling bin the freed memory belongs to.
			defaultHeap.poolLock.lock();
			// TODO: Place the allocation in the correct recycling bin using a linked list.
			defaultHeap.poolLock.unlock();
		}
		defaultHeap->poolLock.unlock();
	}
}
