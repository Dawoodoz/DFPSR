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

#ifndef DFPSR_HEAP
#define DFPSR_HEAP

#include "SafePointer.h"

namespace dsr {
	// Allocate memory in the heap.
	//   The size argument is the minimum number of bytes to allocate, but the result may give you more than you asked for.
	// Post-condition: Returns pointers to the payload and header.
	UnsafeAllocation heap_allocate(uint64_t minimumSize);

	// Pre-condition: The allocation pointer must point to the start of a payload allocated using heap_allocate, no offsets nor other allocators allowed.
	// Post-condition: Returns the number of available bytes in the allocation.
	//                 You may not read a single byte outside of it, because it might include padding that ends at uneven addresses.
	//                 To use more memory than requested, you must round it down to whole elements.
	//                 If the element's size is a power of two, you can pre-compute a bit mask using memory_createAlignmentAndMask for rounding down.
	uint64_t heap_getAllocationSize(uint8_t const * const allocation);

	// Pre-condition: The allocation pointer must point to the start of a payload allocated using heap_allocate, no offsets nor other allocators allowed.
	// Post-condition: Returns a pointer to the heap allocation's header, which is used to construct safe pointers.
	AllocationHeader *heap_getHeader(uint8_t const * const allocation);

	// Only a pointer is needed, so that it can be sent as a function pointer to X11.
	// TODO: Use the allocation head's alignment as the minimum alignment by combining the masks in compile time.
	//       Then it is possible to place the padded allocation header for heap memory at a fixed offset from the allocation start, so that the head can be accessed.
	//       No extra offsets are allowed on the pointer used to free the memory.
	// TODO: Have a global variable containing the default memory pool.
	//       When it is destructed, all allocations that are empty will be freed and a termination flag will be enabled so that any more allocations being freed after it will free the memory themselves.
	void heap_free(uint8_t * const allocation);
}

#endif
