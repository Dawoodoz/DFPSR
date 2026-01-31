// zlib open source license
//
// Copyright (c) 2018 to 2025 David Forsgren Piuva
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

#ifndef DFPSR_API_BUFFER
#define DFPSR_API_BUFFER

#include <cstdint>
#include <memory>
#include "../base/SafePointer.h"
#include "../settings.h"
#include "../base/Handle.h"

// The types of buffers to consider when designing algorithms:
// * Null handle suggesting that there is nothing, such as when loading a file failed.
//     Size does not exist, but is substituted with zero when asked.
//     buffer_exists(Buffer()) == false
//     buffer_dangerous_getUnsafeData(Buffer()) == nullptr
//     buffer_getSize(Buffer()) == 0
// * Empty head, used when loading a file worked but the file itself contained no data.
//     Size equals zero, but stored in the head.
//     Empty buffer heads will be reused when cloning, because they do not share any side-effects
//       when there is no shared data allocation and replacing the destructor will be blocked.
//     buffer_exists(buffer_create(0)) == true
//     buffer_dangerous_getUnsafeData(buffer_create(0)) == nullptr
//     buffer_getSize(buffer_create(0)) == 0
// * Buffer containing data, when the file contained data.
//     When bytes is greater than zero.
//     buffer_exists(buffer_create(bytes)) == true
//     buffer_dangerous_getUnsafeData(buffer_create(x)) == zeroedData
//     buffer_getSize(buffer_create(bytes)) == bytes

namespace dsr {
	using Buffer = Handle<uint8_t>;

	// Allocate a Buffer without padding,
	//   The newSize argument should not include any padding.
	//   The memory is allocated in whole aligned blocks of DSR_MAXIMUM_ALIGNMENT and buffer_getSafeData padds out the SafePointer region to the maximum alignment.
	// Side-effect: Creates a new buffer containing newSize bytes.
	// Post-condition: Returns the new buffer, which is initialized to zeroes.
	Buffer buffer_create(intptr_t newSize);

	// Allocate a Buffer with padding.
	// The buffer always align the start with heap alignment, but this function makes sure that paddToAlignment does not exceed heap alignment.
	// Pre-condition: paddToAlignment <= heap_getHeapAlignment()
	Buffer buffer_create(intptr_t newSize, uintptr_t paddToAlignment, bool zeroed = true);

	// Sets the allocation's destructor, to be called when there are no more reference counted pointers to the buffer.
	//   The destructor is not responsible for freeing the memory allocation itself, only calling destructors in the content.
	// Pre-condition: The buffer exists.
	void buffer_replaceDestructor(Buffer &buffer, const HeapDestructor& newDestructor);

	// Returns true iff buffer exists, even if it is empty without any data allocation.
	inline bool buffer_exists(const Buffer &buffer) {
		return buffer.isNotNull();
	}

	// Returns a clone of the buffer.
	// Giving an empty handle returns an empty handle.
	// If the old buffer's alignment exceeds DSR_DEFAULT_ALIGNMENT, the alignment will be inherited.
	// The resulting buffer will always be aligned by at least DSR_DEFAULT_ALIGNMENT, even if the old buffer had no alignment.
	Buffer buffer_clone(const Buffer &buffer);

	// Returns the buffer's size in bytes, as given when allocating it excluding allocation padding.
	// Returns zero if buffer doesn't exist or has no data allocated.
	intptr_t buffer_getSize(const Buffer &buffer);

	// Returns the number of reference counted handles to the buffer, or 0 if the buffer does not exist.
	intptr_t buffer_getUseCount(const Buffer &buffer);

	// Returns a raw pointer to the data.
	// An empty handle or buffer of length zero without data will return nullptr.
	uint8_t* buffer_dangerous_getUnsafeData(const Buffer &buffer);

	// A wrapper for getting a bound-checked pointer of the correct element type.
	// The name must be an ascii encoded constant literal.
	// Returns a safe null pointer if buffer does not exist or there is no data allocation.
	template <typename T>
	SafePointer<T> buffer_getSafeData(const Buffer &buffer, const char* name) { return buffer.getSafe<T>(name); }

	// Set all bytes to the same value.
	// Pre-condition: buffer exists, or else an exception is thrown to warn you.
	//   If the buffer has a head but no data allocation, the command will be ignored because there are no bytes to set.
	void buffer_setBytes(const Buffer &buffer, uint8_t value);
}

#endif
