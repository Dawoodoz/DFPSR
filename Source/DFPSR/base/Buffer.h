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

#ifndef DFPSR_BUFFER
#define DFPSR_BUFFER

#include <stdint.h>
#include <memory>
#include <functional>
#include "SafePointer.h"

namespace dsr {

class Buffer {
public:
	const int32_t size; // The actually used data
	const int32_t bufferSize; // The accessible data
private:
	uint8_t *data;
	std::function<void(uint8_t *)> destructor;
public:
	explicit Buffer(int32_t newSize);
	Buffer(int32_t newSize, uint8_t *newData);
	~Buffer();
public:
	void replaceDestructor(const std::function<void(uint8_t *)>& newDestructor);
	// Set all bytes to the same value
	void set(uint8_t value);
	// Get a dangerous pointer to the raw data
	uint8_t *getUnsafeData() {
		return this->data;
	}
	const uint8_t *getUnsafeData() const {
		return this->data;
	}
	// Get a safe pointer to the raw data
	template <typename T>
	SafePointer<T> getSafeData(const char *name) {
		return SafePointer<T>(name, (T*)this->data, this->bufferSize, (T*)this->data);
	}
	template <typename T>
	const SafePointer<T> getSafeData(const char *name) const {
		return SafePointer<T>(name, (T*)this->data, this->bufferSize, (T*)this->data);
	}
	// Get a part of the buffer
	template <typename T>
	SafePointer<T> getSafeSlice(const char *name, int offset, int size) {
		return SafePointer<T>(name, (T*)this->data, this->bufferSize, (T*)this->data).slice(name, offset, size);
	}
	std::shared_ptr<Buffer> clone() const;
	static std::shared_ptr<Buffer> create(int32_t newSize);
	static std::shared_ptr<Buffer> create(int32_t newSize, uint8_t *newData);
	// No implicit copies, only pass by reference or pointer
	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;
};

}

#endif

