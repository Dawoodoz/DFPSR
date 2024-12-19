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

namespace dsr {
	struct HeapHeader : public AllocationHeader {
		
	};
	// The heap can have memory freed after its own destruction by telling the remaining allocations to clean up after themselves.
	struct Heap {
		HeapHeader *topHeap;
		uint8_t *data = nullptr;		
		bool terminating = false;
		Heap() {
			this->data = malloc(16 * 1024 * 1024);
			// TODO: Create an empty top allocation inside of the memory and point to it.
			//       Then smaller requests will perform a bisection of the available space using a tree structure.
		}
		~Heap() {
			this->terminating = true;
			free(this->data);
		}
	};

	// Global mutex
	static std::mutex heapLock;
	static Heap defaultHeap;

	UnsafeAllocation heap_allocate(uint64_t paddedSize, uintptr_t alignmentAndMask) {
		heapLock.lock();
		// TODO: Allocate memory in one of the available bins.
		heapLock.unlock();
	}

	void heap_free(uint8_t *allocation) {
		heapLock.lock();
		// TODO: Find the header at a fixed offset and release the allocation.
		//       The length can be used to clear a whole region of memory, including any child allocations within.
		heapLock.unlock();
	}

}
