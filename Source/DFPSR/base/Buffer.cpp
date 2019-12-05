// zlib open source license
//
// Copyright (c) 2017 to 2019 David Forsgren Piuva
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

#include "Buffer.h"
#include "../math/scalar.h"

using namespace dsr;

// buffer_alignment must be a power of two for buffer_alignment_mask to work
static const int buffer_alignment = 16;
static const uintptr_t buffer_alignment_mask = ~((uintptr_t)(buffer_alignment - 1));

// If this C++ version additionally includes the C11 features then we may assume that aligned_alloc is available
#ifdef _ISOC11_SOURCE
	// Allocate data of newSize and write the corresponding destructor function to targetDestructor
	static uint8_t* buffer_allocate(int32_t newSize, std::function<void(uint8_t *)>& targetDestructor) {
		uint8_t* allocation = (uint8_t*)aligned_alloc(buffer_alignment, newSize);
		targetDestructor = [](uint8_t *data) { free(data); };
		return allocation;
	}
#else
	// Allocate data of newSize and write the corresponding destructor function to targetDestructor
	static uint8_t* buffer_allocate(int32_t newSize, std::function<void(uint8_t *)>& targetDestructor) {
		uintptr_t padding = buffer_alignment - 1;
		uint8_t* allocation = (uint8_t*)malloc(newSize + padding);
		uint8_t* aligned = (uint8_t*)(((uintptr_t)allocation + padding) & buffer_alignment_mask);
		uintptr_t offset = allocation - aligned;
		targetDestructor = [offset](uint8_t *data) { free(data - offset); };
		return aligned;
	}
#endif

Buffer::Buffer(int32_t newSize) :
  size(newSize),
  bufferSize(roundUp(newSize, buffer_alignment)) {
	this->data = buffer_allocate(this->bufferSize, this->destructor);
	this->set(0);
}

Buffer::Buffer(int32_t newSize, uint8_t *newData)
: size(newSize), bufferSize(newSize), data(newData), destructor([](uint8_t *data) { free(data); }) {}

Buffer::~Buffer() {
	this->destructor(this->data);
}

void Buffer::replaceDestructor(const std::function<void(uint8_t *)>& newDestructor) {
	this->destructor = newDestructor;
}

void Buffer::set(uint8_t value) {
	memset(this->data, value, this->bufferSize);
}

std::shared_ptr<Buffer> Buffer::clone() const {
	std::shared_ptr<Buffer> newBuffer = std::make_shared<Buffer>(this->size);
	memcpy(newBuffer->data, this->data, this->size);
	return newBuffer;
}

std::shared_ptr<Buffer> Buffer::create(int32_t newSize) {
	return std::make_shared<Buffer>(newSize);
}

std::shared_ptr<Buffer> Buffer::create(int32_t newSize, uint8_t *newData) {
	return std::make_shared<Buffer>(newSize, newData);
}

