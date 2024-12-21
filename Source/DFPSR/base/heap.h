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
	//   size is the number of bytes to allocate.
	UnsafeAllocation heap_allocate(uint64_t size);

	// Only a pointer is needed, so that it can be sent as a function pointer to X11.
	// TODO: Use the allocation head's alignment as the minimum alignment by combining the masks in compile time.
	//       Then it is possible to place the padded allocation header for heap memory at a fixed offset from the allocation start, so that the head can be accessed.
	//       No extra offsets are allowed on the pointer used to free the memory.
	// TODO: Have a global variable containing the default memory pool.
	//       When it is destructed, all allocations that are empty will be freed and a termination flag will be enabled so that any more allocations being freed after it will free the memory themselves.
	void heap_free(uint8_t *allocation);

	// Round a size up to multiples of the heap alignment.
	// Post-condition: Returns size rounded up to the next multiple of heap alignment.
	uint64_t heap_roundSizeUp(uint64_t size);

	/*
	// TODO: Apply additional safety checks when freeing the allocation using SafePointer, making sure that the correct allocation is freed.
	template <typename T>
	void heap_free(SafePointer<T> allocation) {
		
	}
	*/

	// TODO: Create a faster and simpler allocator for things that never live past a function call or frame.
	//       Then you just free the whole thing when done with the allocations.
	//       Good for small images and fixed size heads, but bad for lists being reallocated many times that should recycle memory instead.
	//       A temporary allocator can then only allocate one thing at a time or free all allocations.
	//       A fixed size can guarantee that there is no heap allocation done at runtime.
	//       A dynamic size can have more memory added when needed.
	//       In debug mode, each allocation will be properly destructed.
	//       In release mode, everything is just left as it is.
	//       The caller allocating memory decides if the memory should be cleared or not.
}

#endif
