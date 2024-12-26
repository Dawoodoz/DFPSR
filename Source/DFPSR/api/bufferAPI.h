// zlib open source license
//
// Copyright (c) 2018 to 2023 David Forsgren Piuva
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
#include <functional>
#include "../base/SafePointer.h"
#include "../settings.h"
#include "../base/heap.h"

// The types of buffer handles to consider when designing algorithms:
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
	// A safer replacement for raw memory allocation when you don't need to resize the content.
	// Guarantees that internal addresses will not be invalidated during its lifetime.
	//   Just remember to always keep a handle together with any pointers to the data to prevent the buffer from being freed.
	class BufferImpl;
	using Buffer = std::shared_ptr<BufferImpl>;

	// Side-effect: Creates a new buffer head regardless of newSize, but only allocates a zeroed data allocation if newSize > 0.
	// Post-condition: Returns a handle to the new buffer.
	// Creating a buffer without a size will only allocate the buffer's head referring to null data with size zero.
	Buffer buffer_create(int64_t newSize);
	// The buffer always allocate with DSR_MAXIMUM_ALIGNMENT, but you can check that your requested alignment is not too much.
	Buffer buffer_create(int64_t newSize, int minimumAlignment);

	// Pre-conditions:
	//   newData must be padded and aligned by DSR_MAXIMUM_ALIGNMENT from settings.h if you plan to use it for SIMD or multi-threading.
	//   newSize may not be larger than the size of newData in bytes.
	//     Breaking this pre-condition may cause crashes, so only provide a newData pointer if you know what you are doing.
	// Side-effect: Creates a new buffer of newSize bytes inheriting ownership of newData.
	//   If the given data cannot be freed as a C allocation, replaceDestructor must be called with the special destructor.
	// Post-condition: Returns a handle to the manually constructed buffer.
	Buffer buffer_create(int64_t newSize, uint8_t *newData);

	// Sets the allocation's destructor, to be called when there are no more reference counted pointers to the buffer.
	// Pre-condition: The buffer exists.
	//   If the buffer has a head but no data allocation, the command will be ignored because there is no allocation to delete.
	void buffer_replaceDestructor(const Buffer &buffer, const std::function<void(uint8_t *)>& newDestructor);

	// Returns true iff buffer exists, even if it is empty without any data allocation.
	inline bool buffer_exists(Buffer buffer) {
		return buffer.get() != nullptr;
	}

	// Returns a clone of the buffer.
	// Giving an empty handle returns an empty handle.
	// If the old buffer's alignment exceeds DSR_DEFAULT_ALIGNMENT, the alignment will be inherited.
	// The resulting buffer will always be aligned by at least DSR_DEFAULT_ALIGNMENT, even if the old buffer had no alignment.
	Buffer buffer_clone(const Buffer &buffer);

	// Returns the buffer's size in bytes, as given when allocating it excluding allocation padding.
	// Returns zero if buffer doesn't exist or has no data allocated.
	int64_t buffer_getSize(const Buffer &buffer);

	// Returns the number of reference counted handles to the buffer, or 0 if the buffer does not exist.
	int64_t buffer_getUseCount(const Buffer &buffer);

	// Returns a raw pointer to the data.
	// An empty handle or buffer of length zero without data will return nullptr.
	uint8_t* buffer_dangerous_getUnsafeData(const Buffer &buffer);

	// A wrapper for getting a bound-checked pointer of the correct element type.
	//   Only cast to trivially packed types with power of two dimensions so that the compiler does not add padding.
	// The name must be an ansi encoded constant literal, because each String contains a Buffer which would cause a cyclic dependency.
	// Returns a safe null pointer if buffer does not exist or there is no data allocation.
	template <typename T>
	SafePointer<T> buffer_getSafeData(const Buffer &buffer, const char* name) {
		if (!buffer_exists(buffer)) {
			return SafePointer<T>();
		} else {
			uint8_t *data = buffer_dangerous_getUnsafeData(buffer);
			return SafePointer<T>(name, (T*)data, buffer_getSize(buffer), (T*)data, heap_getHeader(data));
		}
	}

	// Set all bytes to the same value.
	// Pre-condition: buffer exists, or else an exception is thrown to warn you.
	//   If the buffer has a head but no data allocation, the command will be ignored because there are no bytes to set.
	void buffer_setBytes(const Buffer &buffer, uint8_t value);
}

#endif
