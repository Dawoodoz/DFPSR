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
	// How many bytes that are allocated directly in thread local memory.
	static const uint64_t VIRTUAL_STACK_SIZE = 262144;
	static const int MAX_EXTRA_STACKS = 63;

	static const uintptr_t stackHeaderPaddedSize = memory_getPaddedSize<AllocationHeader>();
	static const uintptr_t stackHeaderAlignmentAndMask = memory_createAlignmentAndMask((uintptr_t)alignof(AllocationHeader));

	struct StackMemory {
		uint8_t *top = nullptr; // The stack pointer is here when completely full.
		uint8_t *stackPointer = nullptr; // The virtual stack pointer.
		uint8_t *bottom = nullptr; // The stack pointer is here when empty.
	};

	// The first block of stack memory in stread local memory.
	struct FixedStackMemory : public StackMemory {
		uint8_t data[VIRTUAL_STACK_SIZE];
		FixedStackMemory() {
			this->top = this->data;
			this->stackPointer = this->data + VIRTUAL_STACK_SIZE;
			this->bottom = this->data + VIRTUAL_STACK_SIZE;
		}
	};

	// Additional stacks in heap memory.
	struct DynamicStackMemory : public StackMemory {
		~DynamicStackMemory() {
			if (this->top != nullptr) {
				free(this->top);
			}
		}
	};

	// Returns the size of the allocation including alignment.
	inline uint64_t increaseStackPointer(uint8_t *&pointer, uint64_t paddedSize, uintptr_t alignmentAndMask) {
		// Add the padded payload and align.
		uintptr_t oldAddress = (uintptr_t)pointer;
		uintptr_t newAddress = (oldAddress - paddedSize) & alignmentAndMask;
		pointer = (uint8_t*)newAddress;
		return oldAddress - newAddress;
	}

	inline void decreaseStackPointer(uint8_t *&pointer, uint64_t totalSize) {
		// Remove the data and alignment.
		pointer += totalSize;
	}

	static UnsafeAllocation stackAllocate(StackMemory& stack, uint64_t paddedSize, uintptr_t alignmentAndMask) {
		uint8_t *newStackPointer = stack.stackPointer;
		// Allocate memory for payload.
		uint64_t payloadTotalSize = increaseStackPointer(newStackPointer, paddedSize, alignmentAndMask);
		// Get a pointer to the payload.
		uint8_t *data = newStackPointer;
		// Allocate memory for header.
		uint64_t headerTotalSize = increaseStackPointer(newStackPointer, stackHeaderPaddedSize, stackHeaderAlignmentAndMask);
		// Check that we did not run out of memory.
		if (newStackPointer < stack.top) {
			// Not enough space.
			return UnsafeAllocation(nullptr, nullptr);
		} else {
			stack.stackPointer = newStackPointer;
			// Write the header to memory.
			AllocationHeader *header = (AllocationHeader*)(stack.stackPointer);
			*header = AllocationHeader(payloadTotalSize + headerTotalSize, true);
			// Clear the new allocation for determinism.
			std::memset((void*)data, 0, payloadTotalSize);
			// Return a pointer to the payload.
			return UnsafeAllocation(data, header);
		}
	}

	thread_local FixedStackMemory fixedMemory; // Index -1
	thread_local DynamicStackMemory dynamicMemory[MAX_EXTRA_STACKS]; // Index 0..MAX_EXTRA_STACKS-1
	thread_local int32_t stackIndex = -1;

	UnsafeAllocation virtualStack_push(uint64_t paddedSize, uintptr_t alignmentAndMask) {
		if (stackIndex < 0) {
			UnsafeAllocation result = stackAllocate(fixedMemory, paddedSize, alignmentAndMask);
			// Check that we did not run out of memory.
			if (result.data == nullptr) {
				// Not enough space in thread local memory. Moving to the first dynamic stack.
				stackIndex = 0;
				goto allocateDynamic;
			} else {
				// Return a pointer to the payload.
				return result;
			}
		}
		allocateDynamic:
		// We should only reach this place if allocating in dynamic stack memory.
		assert(stackIndex >= 0);
		// Never go above the maximum index.
		assert(stackIndex < MAX_EXTRA_STACKS);
		// Allocate memory in the dynamic stack if not yet allocated.
		if (dynamicMemory[stackIndex].top == nullptr) {
			uint64_t regionSize = 16777216 * (1 << stackIndex);
			if (paddedSize * 4 > regionSize) {
				regionSize = paddedSize * 4;
			}
			uint8_t *newMemory = (uint8_t*)malloc(regionSize);
			if (newMemory == nullptr) {
				throwError(U"Failed to allocate ", regionSize, U" bytes of heap memory for expanding the virtual stack when trying to allocate ", paddedSize, " bytes!\n");
				return UnsafeAllocation(nullptr, nullptr);
			} else {
				// Keep the new allocation.
				dynamicMemory[stackIndex].top = newMemory;
				// Start from the back of the new allocation.
				dynamicMemory[stackIndex].stackPointer = newMemory + regionSize;
				dynamicMemory[stackIndex].bottom = newMemory + regionSize;
			}
		}
		assert(dynamicMemory[stackIndex].stackPointer != nullptr);
		// Allocate memory.
		UnsafeAllocation result = stackAllocate(dynamicMemory[stackIndex], paddedSize, alignmentAndMask);
		if (result.data == nullptr) {
			if (stackIndex >= MAX_EXTRA_STACKS - 1) {
				throwError(U"Exceeded MAX_EXTRA_STACKS to allocate more heap memory for a thread local virtual stack!\n");
				return UnsafeAllocation(nullptr, nullptr);
			} else {
				stackIndex++;
				goto allocateDynamic;
			}
		} else {
			// Return a pointer to the payload.
			return result;
		}
	}

	// Deallocates the topmost allocation in the stack or returns false if it does not contain any more allocations.
	static bool stackDeallocate(StackMemory& stack) {
		if (stack.stackPointer + stackHeaderPaddedSize > stack.bottom) {
			// If the allocated memory does not fit a header, then it is empty.
			return false;
		} else {
			// Read the header.
			AllocationHeader header = *((AllocationHeader*)stack.stackPointer);
			// Overwrite the header.
			*((AllocationHeader*)stack.stackPointer) = AllocationHeader();
			// Deallocate both header and payload using the stored total size.
			decreaseStackPointer(stack.stackPointer, header.totalSize);
			return true;
		}
	}

	void virtualStack_pop() {
		if (stackIndex < 0) {
			if (!stackDeallocate(fixedMemory)) {
				throwError(U"No more stack memory to pop!\n");
			}
		} else {
			if (!stackDeallocate(dynamicMemory[stackIndex])) {
				throwError(U"The virtual stack has been corrupted!\n");
			} else {
				// If the bottom has been reached then go to the lower stack.
				if (dynamicMemory[stackIndex].stackPointer >= dynamicMemory[stackIndex].bottom) {
					stackIndex--;
				}
			}
		}
	}

}
