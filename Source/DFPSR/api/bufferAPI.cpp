// zlib open source license
//
// Copyright (c) 2019 to 2025 David Forsgren Piuva
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

#include <fstream>
#include "bufferAPI.h"
#include "stringAPI.h"
#include "../implementation/math/scalar.h"
#include "../base/SafePointer.h"
#include "../base/heap.h"

namespace dsr {

Buffer buffer_create(intptr_t newSize) {
	if (newSize < 0) newSize = 0;
	// Allocate head and data.
	return handle_createArray<uint8_t>(AllocationInitialization::Zeroed, (uintptr_t)newSize);
}

Buffer buffer_create(intptr_t newSize, uintptr_t paddToAlignment, bool zeroed) {
	if (newSize < 0) newSize = 0;
	if (paddToAlignment > heap_getHeapAlignment()) {
		throwError(U"Maximum alignment exceeded when creating a buffer!\n");
		return Handle<uint8_t>();
	} else {
		return handle_createArray<uint8_t>(zeroed ? AllocationInitialization::Zeroed : AllocationInitialization::Uninitialized, memory_getPaddedSize((uintptr_t)newSize, paddToAlignment));
	}
}

void buffer_replaceDestructor(Buffer &buffer, const HeapDestructor& newDestructor) {
	if (!buffer_exists(buffer)) {
		throwError(U"buffer_replaceDestructor: Cannot replace destructor for a buffer that don't exist.\n");
	} else {
		heap_setAllocationDestructor(buffer.getUnsafe(), newDestructor);
	}
}

// TODO: Create clone and reallocation methods in heap.h to handle object lifetime in a reusable way.
Buffer buffer_clone(const Buffer &buffer) {
	if (!buffer_exists(buffer)) {
		return Handle<uint8_t>();
	} else {
		uintptr_t size = buffer.getUsedSize();
		if (size == 0) {
			// Buffers of zero elements are reused with reference counting.
			return buffer;
		} else {
			// Allocate new memory without setting it to zero, before cloning data into it.
			Buffer result = handle_createArray<uint8_t>(AllocationInitialization::Uninitialized, size);
			SafePointer<const uint8_t> source = buffer_getSafeData<const uint8_t>(buffer, "Buffer cloning source");
			SafePointer<uint8_t> target = buffer_getSafeData<uint8_t>(result, "Buffer cloning target");
			safeMemoryCopy(target, source, size);
			return result;
		}
	}
}

intptr_t buffer_getSize(const Buffer &buffer) {
	return buffer.getUsedSize();
}

intptr_t buffer_getUseCount(const Buffer &buffer) {
	return buffer.getUseCount();
}

uint8_t* buffer_dangerous_getUnsafeData(const Buffer &buffer) {
	return buffer.getUnsafe();
}

void buffer_setBytes(const Buffer &buffer, uint8_t value) {
	if (buffer.isNull()) {
		throwError(U"buffer_setBytes: Can not set bytes for a buffer that does not exist.\n");
	} else {
		uintptr_t size = buffer.getUsedSize();
		if (size > 0) {
			SafePointer<uint8_t> target = buffer_getSafeData<uint8_t>(buffer, "Buffer set target");
			safeMemorySet(target, value, size);
		}
	}
}

}
