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
#include <thread>
#include "../api/stringAPI.h"

namespace dsr {
	// A structure that is placed in front of each stack allocation while allocating backwards along decreasing addresses to allow aligning memory quickly using bit masking.
	struct StackAllocationHeader {
		uint32_t totalSize; // Size of both header and payload.
		#ifdef SAFE_POINTER_CHECKS
		uint32_t identity; // A unique identifier that can be used to reduce the risk of using a block of memory after it has been freed.
		#endif
		StackAllocationHeader(uint32_t totalSize);
	};

	// How many bytes that are allocated directly in thread local memory.
	static const size_t VIRTUAL_STACK_SIZE = 262144;
	// How many bytes are reserved for the head.
	static const size_t ALLOCATION_HEAD_SIZE = memory_getPaddedSize<StackAllocationHeader>();
	
	static const uintptr_t stackHeaderPaddedSize = memory_getPaddedSize<StackAllocationHeader>();
	static const uintptr_t stackHeaderAlignmentAndMask = memory_createAlignmentAndMask((uintptr_t)alignof(StackAllocationHeader));

	#ifdef SAFE_POINTER_CHECKS
		// In debug mode, each new thread creates a hash from its own identity to catch most of the memory errors in debug mode.
		std::hash<std::thread::id> hasher;
		thread_local uint32_t threadIdentity = hasher(std::this_thread::get_id());
		//   To check the allocation identity, subtract the padded size of the header from the base pointer, cast to the head type and compare to the pointer's identity.
		thread_local uint32_t nextIdentity = threadIdentity;
	#endif
	StackAllocationHeader::StackAllocationHeader(uint32_t totalSize) : totalSize(totalSize) {
		#ifdef SAFE_POINTER_CHECKS
			// No identity may be zero, because identity zero is no identity.
			if (nextIdentity == 0) nextIdentity++;
			this->identity = nextIdentity;
			nextIdentity++;
		#endif
	}

	struct StackMemory {
		uint8_t data[VIRTUAL_STACK_SIZE];
		uint8_t *top = nullptr;
		StackMemory() {
			this->top = this->data + VIRTUAL_STACK_SIZE;
		}
	};
	thread_local StackMemory virtualStack;

	// Returns the size of the allocation including alignment.
	inline uint64_t increaseTop(uint64_t paddedSize, uintptr_t alignmentAndMask) {
		// Add the padded payload and align.
		uintptr_t oldAddress = (uintptr_t)virtualStack.top;
		uintptr_t newAddress = (oldAddress - paddedSize) & alignmentAndMask;
		virtualStack.top = (uint8_t*)newAddress;
		return oldAddress - newAddress;
	}

	inline void decreaseTop(uint64_t totalSize) {
		// Remove the data and alignment.
		virtualStack.top += totalSize;
	}

	uint8_t *virtualStack_push(uint64_t paddedSize, uintptr_t alignmentAndMask) {
		uint8_t *oldTop = virtualStack.top;
		// Allocate memory for payload.
		uint64_t payloadTotalSize = increaseTop(paddedSize, alignmentAndMask);
		// Get a pointer to the payload.
		uint8_t *result = virtualStack.top;
		// Allocate memory for header.
		uint64_t headerTotalSize = increaseTop(stackHeaderPaddedSize, stackHeaderAlignmentAndMask);
		// Check that we did not run out of memory.
		if (virtualStack.top < virtualStack.data) {
			// TODO: Expand automatically using heap memory instead of crashing.
			throwError(U"Ran out of virtual stack memory to allocate when trying to allocate ", paddedSize, U" bytes!\n");
			virtualStack.top = oldTop;
			return nullptr;
		} else {
			// Write the header to memory.
			*((StackAllocationHeader*)virtualStack.top) = StackAllocationHeader(payloadTotalSize + headerTotalSize);
			// Clear the new allocation for determinism.
			std::memset((char*)result, 0, payloadTotalSize);
			// Return a pointer to the payload.
			return result;
		}
	}

	void virtualStack_pop() {
		if (virtualStack.top + stackHeaderPaddedSize > virtualStack.data + VIRTUAL_STACK_SIZE) {
			throwError(U"No more stack memory to pop!\n");
		} else {
			// Read the header.
			StackAllocationHeader header = *((StackAllocationHeader*)virtualStack.top);
			// Deallocate both header and payload using the stored total size.
			decreaseTop(header.totalSize);
		}
	}

}
