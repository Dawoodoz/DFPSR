
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

// A fixed size collection of elements initialized to the same default value.
//   Unlike Buffer, Array is a value type, so be careful not to pass it by value unless you intend to clone its content.
template <typename T>
class Array {
private:
	int64_t elementCount = 0;
	T *elements = nullptr;
public:
	// Constructor
	Array(const int64_t newLength, const T& defaultValue)
	  : elementCount(newLength) {
  		impl_nonZeroLengthCheck(newLength, "New array length");
		this->elements = new T[newLength];
		for (int64_t index = 0; index < newLength; index++) {
			this->elements[index] = defaultValue;
		}
	}
	// Clonable by default!
	//   Be very careful not to accidentally pass an Array by value instead of reference,
	//   otherwise your side-effects might write to a temporary copy
	//   or time is wasted to clone an Array every time you look something up.
	Array(const Array<T>& source) {
		// Allocate to the same size as source.
		this->elements = new T[source.elementCount];
		this->elementCount = source.elementCount;
		// Copy elements from source.
		for (int64_t e = 0; e < this->elementCount; e++) {
			// Assign one element at a time, so that objects can be copy constructed.
			//   If the element type T is trivial and does not require calling constructors, using safeMemoryCopy with SafePointer will be much faster than using Array<T>.
			this->elements[e] = source.elements[e];
		}
	};
	// When assigning to the array, memory can be reused when the size is the same.
	Array& operator=(const Array<T>& source) {
		// Reallocate to the same size as source if needed.
		if (this->elementCount != source.elementCount) {
			if (this->elements) delete[] this->elements;
			this->elements = new T[source.elementCount];
		}
		this->elementCount = source.elementCount;
		// Copy elements from source.
		for (int64_t e = 0; e < this->elementCount; e++) {
			// Assign one element at a time, so that objects can be copy constructed.
			//   If the element type T is trivial and does not require calling constructors, using safeMemoryCopy with SafePointer will be much faster than using Array<T>.
			this->elements[e] = source.elements[e];
		}
		return *this;
	};
	// Destructor
	~Array() { if (this->elements) delete[] this->elements; }
	// Element access
	T& operator[] (const int64_t index) {
		impl_baseZeroBoundCheck(index, this->length(), "Array index");
		return this->elements[index];
	}
	const T& operator[] (const int64_t index) const {
		impl_baseZeroBoundCheck(index, this->length(), "Array index");
		return this->elements[index];
	}
	int64_t length() const {
		return this->elementCount;
	}
};

}

#endif
