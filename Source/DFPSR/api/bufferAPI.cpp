// zlib open source license
//
// Copyright (c) 2019 to 2023 David Forsgren Piuva
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
#include <cstdlib>
#include "bufferAPI.h"
#include "stringAPI.h"
#include "../math/scalar.h"
#include "../base/simd.h"

namespace dsr {

// Hidden type

class BufferImpl {
public:
	// A Buffer cannot have a name, because each String contains a buffer
	const int64_t size; // The actually used data
	const int64_t bufferSize; // The accessible data
	const int alignment;
	uint8_t *data;
	std::function<void(uint8_t *)> destructor;
public:
	// Create head without data.
	BufferImpl();
	// Create head with newly allocated data.
	explicit BufferImpl(int64_t newSize, int alignment);
	// Create head with inherited data.
	BufferImpl(int64_t newSize, uint8_t *newData);
	~BufferImpl();
public:
	// No implicit copies, only pass using the Buffer handle
	BufferImpl(const BufferImpl&) = delete;
	BufferImpl& operator=(const BufferImpl&) = delete;
};

// Internal methods

// Get the largest alignment and confirm that it is a power of two.
static int getFinalAlignment(int requestedAlignment) {
	// Find any power of two alignment divisible by both requestedAlignment and DSR_DEFAULT_ALIGNMENT
	int largestAlignment = max(requestedAlignment, DSR_DEFAULT_ALIGNMENT);
	for (uint32_t e = 0; e < 32; e++) {
		if (1 << e == largestAlignment) return largestAlignment;
	}
	return -1;
}

// If this C++ version additionally includes the C11 features then we may assume that aligned_alloc is available
#ifdef _ISOC11_SOURCE
	// Allocate data of newSize and write the corresponding destructor function to targetDestructor
	static uint8_t* buffer_allocate(int64_t newSize, int alignment, std::function<void(uint8_t *)>& targetDestructor) {
		uint8_t* allocation = (uint8_t*)aligned_alloc(alignment, newSize);
		targetDestructor = [](uint8_t *data) { free(data); };
		return allocation;
	}
#else
	// Allocate data of newSize and write the corresponding destructor function to targetDestructor
	static uint8_t* buffer_allocate(int64_t newSize, int alignment, std::function<void(uint8_t *)>& targetDestructor) {
		uintptr_t padding = alignment - 1;
		uint8_t* allocation = (uint8_t*)malloc(newSize + padding);
		uintptr_t buffer_alignment_mask = ~((uintptr_t)(alignment - 1));
		uint8_t* aligned = (uint8_t*)(((uintptr_t)allocation + padding) & buffer_alignment_mask);
		uintptr_t offset = allocation - aligned;
		targetDestructor = [offset](uint8_t *data) { free(data - offset); };
		return aligned;
	}
#endif

BufferImpl::BufferImpl() : size(0), bufferSize(0), alignment(DSR_DEFAULT_ALIGNMENT), data(nullptr) {}

BufferImpl::BufferImpl(int64_t newSize, int alignment) :
  size(newSize),
  bufferSize(roundUp(newSize, alignment)),
  alignment(alignment) {
	this->data = buffer_allocate(this->bufferSize, alignment, this->destructor);
	if (this->data == nullptr) {
		throwError(U"Failed to allocate buffer of ", newSize, " bytes!\n");
	}
	memset(this->data, 0, this->bufferSize);
}

BufferImpl::BufferImpl(int64_t newSize, uint8_t *newData)
: size(newSize), bufferSize(newSize), alignment(1), data(newData), destructor([](uint8_t *data) { free(data); }) {}

BufferImpl::~BufferImpl() {
	if (this->data) {
		this->destructor(this->data);
	}
}

// API

Buffer buffer_clone(const Buffer &buffer) {
	if (!buffer_exists(buffer)) {
		// If the original buffer does not exist, just return another null handle.
		return Buffer();
	} else {
		if (buffer->size <= 0) {
			// No need to clone when there is no shared data.
			return buffer;
		} else {
			// Clone the data so that content of the allocations can be modified individually without affecting each other.
			Buffer newBuffer = std::make_shared<BufferImpl>(buffer->size, max(buffer->alignment, DSR_DEFAULT_ALIGNMENT));
			memcpy(newBuffer->data, buffer->data, buffer->size);
			return newBuffer;
		}
	}
}

Buffer buffer_create(int64_t newSize, int minimumAlignment) {
	if (newSize < 0) newSize = 0;
	if (newSize == 0) {
		// Allocate empty head to indicate that an empty buffer exists.
		return std::make_shared<BufferImpl>();
	} else {
		// Allocate head and data.
		int finalAlignment = getFinalAlignment(minimumAlignment);
		if (finalAlignment != -1) {
			return std::make_shared<BufferImpl>(newSize, finalAlignment);
		} else {
			throwError(U"buffer_create: Minimum alignment ", minimumAlignment, " is not a power of two!\n");
			return Buffer(); // Invalid alignment argument was not a power of two.
		}
	}
}

Buffer buffer_create(int64_t newSize, uint8_t *newData) {
	if (newSize < 0) newSize = 0;
	return std::make_shared<BufferImpl>(newSize, newData);
}

void buffer_replaceDestructor(const Buffer &buffer, const std::function<void(uint8_t *)>& newDestructor) {
	if (!buffer_exists(buffer)) {
		throwError(U"buffer_replaceDestructor: Cannot replace destructor for a buffer that don't exist.\n");
	} else if (buffer->bufferSize > 0) {
		buffer->destructor = newDestructor;
	}
}

int64_t buffer_getSize(const Buffer &buffer) {
	if (!buffer_exists(buffer)) {
		return 0;
	} else {
		return buffer->size;
	}
}

int64_t buffer_getUseCount(const Buffer &buffer) {
	if (!buffer_exists(buffer)) {
		return 0;
	} else {
		return buffer.use_count();
	}
}

uint8_t* buffer_dangerous_getUnsafeData(const Buffer &buffer) {
	if (!buffer_exists(buffer)) {
		return nullptr;
	} else {
		return buffer->data;
	}
}

void buffer_setBytes(const Buffer &buffer, uint8_t value) {
	if (!buffer_exists(buffer)) {
		throwError(U"buffer_setBytes: Cannot set bytes for a buffer that don't exist.\n");
	} else if (buffer->bufferSize > 0) {
		memset(buffer->data, value, buffer->bufferSize);
	}
}

}
