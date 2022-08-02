
// zlib open source license
//
// Copyright (c) 2018 to 2019 David Forsgren Piuva
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

#ifndef DFPSR_COLLECTION_ARRAY
#define DFPSR_COLLECTION_ARRAY

#include "collections.h"

namespace dsr {

// The simplest possible automatically deallocating array with bound checks.
//   Indices use signed indices, which can be used directly from high-level algorithms.
// Because std::vector is a list of members, not a fixed size array of values.
//   Using a list instead of an array makes the code both dangerous and unreadable.
//   Using unsigned indices will either force dangerous casting from signed, or prevent
//   the ability to loop backwards without crashing when the x < 0u criteria cannot be met.
template <typename T>
class Array {
private:
	const int32_t elementCount;
	T *elements = nullptr;
public:
	// Constructor
	Array(const int32_t newLength, const T& defaultValue)
	  : elementCount(newLength) {
  		impl_nonZeroLengthCheck(newLength, "New array length");
		this->elements = new T[newLength];
		for (int32_t index = 0; index < newLength; index++) {
			this->elements[index] = defaultValue;
		}
	}
	// No implicit copies, only pass by reference
	Array(const Array&) = delete;
	Array& operator=(const Array&) = delete;
	// Destructor
	~Array() { delete[] this->elements; }
	// Element access
	T& operator[] (const int32_t index) {
		impl_baseZeroBoundCheck(index, this->length(), "Array index");
		return this->elements[index];
	}
	const T& operator[] (const int32_t index) const {
		impl_baseZeroBoundCheck(index, this->length(), "Array index");
		return this->elements[index];
	}
	int32_t length() const {
		return this->elementCount;
	}
};

}

#endif

