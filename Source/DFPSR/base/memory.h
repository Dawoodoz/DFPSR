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

#ifndef DFPSR_MEMORY
#define DFPSR_MEMORY

// Safe pointer checks are removed in release mode for speed after having tested the program in debug mode for safety.
#ifndef NDEBUG
	#define SAFE_POINTER_CHECKS
#endif

#include <cstdint>

namespace dsr {
	// A header that is placed next to memory allocations.
	struct AllocationHeader {
		uintptr_t totalSize; // Size of both header and payload.
		#ifdef SAFE_POINTER_CHECKS
			uint64_t threadHash; // Hash of the owning thread identity for thread local memory, 0 for shared memory.
			uint64_t allocationIdentity; // Rotating identity of the allocation, to know if the memory has been freed and reused within a memory allocator.
		#endif
		// Header for freed memory.
		AllocationHeader();
		// Header for allocated memory.
		// threadLocal should be true iff the memory may not be accessed from other threads, such as virtual stack memory.
		AllocationHeader(uintptr_t totalSize, bool threadLocal);
	};

	// A structure used to allocate memory before placing the content in SafePointer.
	struct UnsafeAllocation {
		uint8_t *data;
		#ifdef SAFE_POINTER_CHECKS
			AllocationHeader *header;
			UnsafeAllocation(uint8_t *data, AllocationHeader *header)
			: data(data), header(header) {}
		#else
			UnsafeAllocation(uint8_t *data, AllocationHeader *header)
			: data(data) {}
		#endif
	};

	// Post-condition: Returns the size of T rounded up by T's own alignment, which becomes the stride between elements in a memory aligned array.
	template <typename T>
	constexpr uint64_t memory_getPaddedSize() {
		uint64_t size = (uint64_t)sizeof(T);
		uint64_t alignment = (uint64_t)alignof(T);
		// Round up with unsigned integers.
		return size + (alignment - 1) - ((size - 1) % alignment);
	}

	// Create a mask for aligning memory in descending address space.
	//   The bitwise and operation "&" between a pointer and this function's result becomes an address rounded down by alignment.
	//     myAlignedPointer = (uintptr_t)myPointer & memory_createAlignmentAndMask(alignment)
	//   By allocating memory from back to front, rounding down can be used for memory alignment.
	// Pre-condition:
	//   alignment is a power of two (1, 2, 4, 8, 16, 32, 64...)
	// Post-condition:
	//   Returns a bit mask for rounding an integer down to the closest multiple of alignment.
	constexpr uintptr_t memory_createAlignmentAndMask(uintptr_t alignment) {
		// alignment = ...00001000...
		// Subtracting one from a power of two gives a mask with ones for the remainder bits.
		// remainder = ...00000111...
		// Then we simply negate the mask to get the alignment mask for rounding down.
		// mask      = ...11111000...
		return ~(alignment - 1);
	}
}

#endif
