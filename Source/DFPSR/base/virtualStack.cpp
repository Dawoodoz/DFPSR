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

#include "virtualStack.h"

namespace dsr {
	// How many bytes that are allocated directly in thread local memory.
	static const size_t DSR_VIRTUAL_STACK_SIZE = 131072;

	// TODO: Allow expanding using recycled heap memory when running out of stack space.
	// TODO: Align the first allocation in address space from unaligned memory.
	//       The easiest way would be to allocate memory in reverse order from the end.
	//       * Subtract the amount of allocated memory from the previous uint8_t pointer.
	//       * Use the pre-defined alignment mask that has zeroes for the rounded bits in the address.
	//       * Place the allocation in the aligned location.
	//       * Store an allocation size integer in front of the allocation to allow freeing.
	//         The integer stores the total size of the size integer, allocation with padded size and alignment padding.
	//         Arrays are allowed to access the padded size of all elements to allow optimizations.
	//       * Store the new stack location pointing at the integer, with a fixed offset from the topmost allocation.
	//       This would also make it easier to unwind the allocations when freeing memory.
	//       * Read the integer pointed to and add it to the pointer.

	struct StackMemory {
		uint8_t data[DSR_VIRTUAL_STACK_SIZE];
		uint64_t stackLocation = 0;
		// TODO: Try to store stack locations between the allocations to avoid heap allocations.
		List<uint64_t> allocationEnds;
	};
	thread_local StackMemory virtualStack;

	uint8_t *virtualStack_push(uint64_t paddedSize, uint64_t alignment) {
		uint64_t oldStackLocation = virtualStack.stackLocation;
		// Align the start location by rounding up and then add padded elements.
		uint64_t startOffset = roundUp(virtualStack.stackLocation, alignment);
		virtualStack.stackLocation = startOffset + paddedSize;
		if (virtualStack.stackLocation > DSR_VIRTUAL_STACK_SIZE) {
			throwError(U"Ran out of stack memory!\n"); // TODO: Expand automatically using more memory blocks instead of crashing.
			return nullptr;
		} else {
			virtualStack.allocationEnds.push(oldStackLocation);
			// Clear the allocation for determinism.
			std::memset((char*)(virtualStack.data + startOffset), 0, paddedSize);
			return virtualStack.data + startOffset;
		}
	}

	void virtualStack_pop() {
		virtualStack.stackLocation = virtualStack.allocationEnds.last();
		virtualStack.allocationEnds.pop();
	}

}
