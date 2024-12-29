﻿// zlib open source license
//
// Copyright (c) 2019 to 2024 David Forsgren Piuva
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
	// Create head without data.
	BufferImpl();
	// Create head with newly allocated data.
	explicit BufferImpl(int64_t newSize);
	// Create head with inherited data.
	BufferImpl(int64_t newSize, uint8_t *newData);
	~BufferImpl();
public:
	// No implicit copies, only pass using the Buffer handle
	BufferImpl(const BufferImpl&) = delete;
	BufferImpl& operator=(const BufferImpl&) = delete;
};

// Internal methods

static uint8_t* buffer_allocate(int64_t newSize, std::function<void(uint8_t *)>& targetDestructor) {
	uint8_t* allocation = heap_allocate(newSize).data;
	targetDestructor = [](uint8_t *data) { heap_free(data); };
	return allocation;
}

BufferImpl::BufferImpl() : size(0), bufferSize(0), data(nullptr) {}

BufferImpl::BufferImpl(int64_t newSize) :
  size(newSize),
  bufferSize(roundUp(newSize, DSR_MAXIMUM_ALIGNMENT)) {
	this->data = buffer_allocate(this->bufferSize, this->destructor);
	if (this->data == nullptr) {
		throwError(U"Failed to allocate buffer of ", newSize, " bytes!\n");
	}
	memset(this->data, 0, this->bufferSize);
}

BufferImpl::BufferImpl(int64_t newSize, uint8_t *newData)
: size(newSize), bufferSize(newSize), data(newData), destructor([](uint8_t *data) { heap_free(data); }) {}

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
			Buffer newBuffer = std::make_shared<BufferImpl>(buffer->size);
			memcpy(newBuffer->data, buffer->data, buffer->size);
			return newBuffer;
		}
	}
}

Buffer buffer_create(int64_t newSize) {
	if (newSize < 0) newSize = 0;
	if (newSize == 0) {
		// Allocate empty head to indicate that an empty buffer exists.
		return std::make_shared<BufferImpl>();
	} else {
		// Allocate head and data.
		return std::make_shared<BufferImpl>(newSize);
	}
}

Buffer buffer_create(int64_t newSize, int minimumAlignment) {
	if (newSize < 0) newSize = 0;
	if (newSize == 0) {
		// Allocate empty head to indicate that an empty buffer exists.
		return std::make_shared<BufferImpl>();
	} else if (minimumAlignment > DSR_MAXIMUM_ALIGNMENT) {
		throwError(U"Maximum alignment exceeded when creating a buffer!\n");
		return Buffer();
	} else {
		// Allocate head and data.
		return std::make_shared<BufferImpl>(newSize);
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
