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

#include "SafePointer.h"
#include "../api/stringAPI.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>

namespace dsr {
	// TODO: Make overloaded versions for signed and unsigned integer types.
	constexpr uint64_t roundUp(uint64_t size, uint64_t alignment) {
		return size + (alignment - 1) - ((size - 1) % alignment);
	}

	template <typename T>
	constexpr uint64_t memory_getPaddedElementSize() {
		return roundUp((uint64_t)sizeof(T), (uint64_t)alignof(T));
	}

	// Allocate memory in the virtual stack owned by the current thread.
	//   paddedSize is the number of bytes to allocate including all elements and internal padding.
	//   alignment is what the start address should be divisible by in bytes, which must be a power of two.
	uint8_t *virtualStack_push(uint64_t paddedSize, uint64_t alignment);

	// A simpler way to get the correct alignment is to allocate a number of elements with a specific type.
	// TODO: Create another function for manual alignment exceeding the type's alignment using another template argument.
	// TODO: Let the address offset be negated and start with the allocation size going down to zero,
	//       so that rounding up addresses can be done by simply masking the least significant bits.
	template <typename T>
	SafePointer<T> virtualStack_push(uint64_t elementCount, const char *name) {
		// Calculate element size and multiply by element count to get the total size.
		uint64_t paddedSize = memory_getPaddedElementSize<T>() * elementCount;
		// Allocate the data with the amount of alignment requested by the element type T.
		uint8_t *data = virtualStack_push(paddedSize, (uint64_t)alignof(T));
		// Return a safe pointer to the allocated data.
		return SafePointer<T>(name, (T*)data, (intptr_t)paddedSize);
	}

	// Free the last allocation from the virtual stack.
	//   Must be called from the same thread that pushed, because virtual stacks are local to their threads.
	void virtualStack_pop();

	// Allocate this array on the stack to automatically free the memory when the scope ends.
	//   Replaces VLA or alloca.
	template <typename T>
	struct VirtualStackAllocation {
		SafePointer<T> data;
		VirtualStackAllocation(uint64_t elementCount)
		: data(virtualStack_push<T>(elementCount, "virtual stack allocation")) {}
		~VirtualStackAllocation() {
			virtualStack_pop();
		}
	};
}
