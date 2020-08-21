// zlib open source license
//
// Copyright (c) 2018 to 2020 David Forsgren Piuva
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

#include <stdint.h>
#include <memory>
#include <functional>
#include "../base/SafePointer.h"

namespace dsr {
	// A safer replacement for raw memory allocation when you don't need to resize the content.
	// Guarantees that internal addresses will not be invalidated during its lifetime.
	//   Just remember to always keep a handle together with any pointers to the data to prevent the buffer from being freed.
	class BufferImpl;
	using Buffer = std::shared_ptr<BufferImpl>;

	// Creates a new buffer of newSize bytes.
	// Pre-condition: newSize > 0
	Buffer buffer_create(int64_t newSize);

	// Creates a new buffer of newSize bytes inheriting ownership of newData.
	//   If the given data cannot be freed as a C allocation, replaceDestructor must be called with the special destructor.
	// Pre-condition: newSize > 0
	Buffer buffer_create(int64_t newSize, uint8_t *newData);

	// Sets the allocation's destructor, to be called when there are no more reference counted pointers to the buffer.
	// Pre-condition: buffer exists
	void buffer_replaceDestructor(Buffer buffer, const std::function<void(uint8_t *)>& newDestructor);

	// Returns true iff buffer exists
	inline bool buffer_exists(Buffer buffer) {
		return buffer.get() != nullptr;
	}

	// Returns a clone of the buffer.
	// Giving an empty handle returns an empty handle
	Buffer buffer_clone(Buffer buffer);

	// Returns the buffer's size in bytes, as given when allocating it excluding allocation padding
	// Returns zero if buffer doesn't exist
	int64_t buffer_getSize(Buffer buffer);

	// Returns a raw pointer to the data.
	// An empty handle will return nullptr.
	uint8_t* buffer_dangerous_getUnsafeData(Buffer buffer);

	// A wrapper for getting a bound-checked pointer of the correct element type.
	//   Only cast to trivially packed types with power of two dimensions so that the compiler does not add padding.
	// The name must be an ansi encoded constant literal, because each String contains a Buffer which would cause a cyclic dependency.
	// Returns a safe null pointer if buffer does not exist.
	template <typename T>
	SafePointer<T> buffer_getSafeData(Buffer buffer, const char* name) {
		if (!buffer_exists(buffer)) {
			return SafePointer<T>();
		} else {
			uint8_t *data = buffer_dangerous_getUnsafeData(buffer);
			return SafePointer<T>(name, (T*)data, buffer_getSize(buffer), (T*)data);
		}
	}

	// Set all bytes to the same value
	// Pre-condition: buffer exists
	void buffer_setBytes(Buffer buffer, uint8_t value);
}

#endif
