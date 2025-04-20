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

#ifndef DFPSR_VIRTUAL_STACK
#define DFPSR_VIRTUAL_STACK

#include "SafePointer.h"

namespace dsr {
	// Allocate memory in the virtual stack owned by the current thread.
	//   paddedSize is the number of bytes to allocate including all elements and internal padding.
	//     paddedSize must be at least 1, but has no rounding requirements.
	//   The start of the allocation is aligned according to alignmentAndMask.
	//     alignmentAndMask should only contain zeroes at the bits to round away for alignment.
	//     alignmentAndMask should be the bitwise negation of the alignment minus one, where the alignment is a power of two.
	//     ~(alignment - 1)
	UnsafeAllocation virtualStack_push(uint64_t paddedSize, uintptr_t alignmentAndMask, const char *name = "Nameless virtual stack allocation");

	// A simpler way to get the correct alignment is to allocate a number of elements with a specific type.
	// Pre-condition:
	//   sizeof(T) % alignof(T) == 0
	template <typename T>
	SafePointer<T> virtualStack_push(uint64_t elementCount, const char *name = "Nameless virtual stack allocation", uintptr_t alignmentAndMask = ~uintptr_t(0u)) {
		// Calculate element size and multiply by element count to get the total size.
		uint64_t paddedSize = sizeof(T) * elementCount;
		// Allocate the data with the amount of alignment requested by the element type T.
		UnsafeAllocation result = virtualStack_push(paddedSize, alignmentAndMask & memory_createAlignmentAndMask((uintptr_t)alignof(T)), name);
		// Return a safe pointer to the allocated data.
		#ifdef SAFE_POINTER_CHECKS
			return SafePointer<T>(result.header, result.header->allocationIdentity, name, (T*)(result.data), (intptr_t)paddedSize);
		#else
			return SafePointer<T>(name, (T*)(result.data), (intptr_t)paddedSize);
		#endif
	}

	// Free the last allocation from the virtual stack.
	//   Must be called from the same thread that pushed, because virtual stacks are local to their threads.
	void virtualStack_pop();

	// Allocate this array on the stack to automatically free the memory when the scope ends.
	//   Replaces Variable Length Arrays (VLA) or alloca.
	template <typename T>
	class VirtualStackAllocation : public SafePointer<T> {
	public:
		VirtualStackAllocation(uint64_t elementCount, const char *name = "Nameless virtual stack allocation", uintptr_t alignmentAndMask = ~uintptr_t(0u))
		: SafePointer<T>(virtualStack_push<T>(elementCount, name, alignmentAndMask)) {}
		~VirtualStackAllocation() {
			virtualStack_pop();
		}
	};

	template <typename T>
	class DestructibleVirtualStackAllocation : public SafePointer<T> {
	public:
		uint64_t elementCount;
		DestructibleVirtualStackAllocation(uint64_t elementCount, const char *name = "Nameless virtual stack allocation", uintptr_t alignmentAndMask = ~uintptr_t(0u))
		: SafePointer<T>(virtualStack_push<T>(elementCount, name, alignmentAndMask)), elementCount(elementCount) {}
		~DestructibleVirtualStackAllocation() {
			// Call destructors.
			for (uint64_t e = 0; e < elementCount; e++) {
				(*this)[e].~T();
			}
			virtualStack_pop();
		}
	};
}

#endif
