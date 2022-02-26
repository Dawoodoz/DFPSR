// zlib open source license
//
// Copyright (c) 2019 to 2020 David Forsgren Piuva
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

namespace dsr {

// Hidden type

class BufferImpl {
public:
	// A Buffer cannot have a name, because each String contains a buffer
	const int64_t size; // The actually used data
	const int64_t bufferSize; // The accessible data
	uint8_t *data;
	std::function<void(uint8_t *)> destructor;
public:
	explicit BufferImpl(int64_t newSize);
	BufferImpl(int64_t newSize, uint8_t *newData);
	~BufferImpl();
public:
	// No implicit copies, only pass using the Buffer handle
	BufferImpl(const BufferImpl&) = delete;
	BufferImpl& operator=(const BufferImpl&) = delete;
};

// Internal methods

// buffer_alignment must be a power of two for buffer_alignment_mask to work
static const int buffer_alignment = 16;

// If this C++ version additionally includes the C11 features then we may assume that aligned_alloc is available
#ifdef _ISOC11_SOURCE
	// Allocate data of newSize and write the corresponding destructor function to targetDestructor
	static uint8_t* buffer_allocate(int64_t newSize, std::function<void(uint8_t *)>& targetDestructor) {
		uint8_t* allocation = (uint8_t*)aligned_alloc(buffer_alignment, newSize);
		targetDestructor = [](uint8_t *data) { free(data); };
		return allocation;
	}
#else
	static const uintptr_t buffer_alignment_mask = ~((uintptr_t)(buffer_alignment - 1));
	// Allocate data of newSize and write the corresponding destructor function to targetDestructor
	static uint8_t* buffer_allocate(int64_t newSize, std::function<void(uint8_t *)>& targetDestructor) {
		uintptr_t padding = buffer_alignment - 1;
		uint8_t* allocation = (uint8_t*)malloc(newSize + padding);
		uint8_t* aligned = (uint8_t*)(((uintptr_t)allocation + padding) & buffer_alignment_mask);
		uintptr_t offset = allocation - aligned;
		targetDestructor = [offset](uint8_t *data) { free(data - offset); };
		return aligned;
	}
#endif

BufferImpl::BufferImpl(int64_t newSize) :
  size(newSize),
  bufferSize(roundUp(newSize, buffer_alignment)) {
	this->data = buffer_allocate(this->bufferSize, this->destructor);
	if (this->data == nullptr) {
		throwError(U"Failed to allocate buffer of ", newSize, " bytes!\n");
	}
	memset(this->data, 0, this->bufferSize);
}

BufferImpl::BufferImpl(int64_t newSize, uint8_t *newData)
: size(newSize), bufferSize(newSize), data(newData), destructor([](uint8_t *data) { free(data); }) {}

BufferImpl::~BufferImpl() {
	this->destructor(this->data);
}

// API

Buffer buffer_clone(const Buffer &buffer) {
	if (!buffer_exists(buffer)) {
		return Buffer();
	} else {
		Buffer newBuffer = std::make_shared<BufferImpl>(buffer->size);
		memcpy(newBuffer->data, buffer->data, buffer->size);
		return newBuffer;
	}
}

Buffer buffer_create(int64_t newSize) {
	if (newSize <= 0) {
		throwError(U"buffer_create: Cannot create buffer of size ", newSize, U".\n");
		return Buffer();
	} else {
		return std::make_shared<BufferImpl>(newSize);
	}
}

Buffer buffer_create(int64_t newSize, uint8_t *newData) {
	if (newSize <= 0) {
		throwError(U"buffer_create: Cannot create buffer of size ", newSize, U" (with existing data).\n");
		return Buffer();
	} else {
		return std::make_shared<BufferImpl>(newSize, newData);
	}
}

void buffer_replaceDestructor(const Buffer &buffer, const std::function<void(uint8_t *)>& newDestructor) {
	if (!buffer_exists(buffer)) {
		throwError(U"buffer_replaceDestructor: Cannot replace destructor for a buffer that don't exist.\n");
	} else {
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
	} else {
		memset(buffer->data, value, buffer->bufferSize);
	}
}

}
