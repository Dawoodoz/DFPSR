
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

#ifndef DFPSR_COLLECTION_ARRAY
#define DFPSR_COLLECTION_ARRAY

#include "collections.h"

namespace dsr {

// A fixed size collection of impl_elements initialized to the same default value.
//   Unlike Buffer, Array is a value type, so be careful not to pass it by value unless you intend to clone its content.
template <typename T>
class Array {
private:
	intptr_t impl_elementCount = 0;
	T *impl_elements = nullptr;
	void impl_free() {
		if (this->impl_elements != nullptr) {
			heap_decreaseUseCount(this->impl_elements);
			this->impl_elements = nullptr;
		}
	}
	// Pre-condition: this->impl_elements == nullptr
	void impl_allocate(intptr_t elementCount) {
		this->impl_elementCount = elementCount;
		UnsafeAllocation newAllocation = heap_allocate(elementCount * sizeof(T), false);
		#ifdef SAFE_POINTER_CHECKS
			heap_setAllocationName(newAllocation.data, "Array allocation");
		#endif
		this->impl_elements = (T*)(newAllocation.data);
		heap_increaseUseCount(newAllocation.header);
	}
	void impl_reallocate(intptr_t elementCount) {
		// Check how much space is available in the target.
		uintptr_t allocationSize = heap_getAllocationSize(this->impl_elements);
		uintptr_t neededSize = elementCount * sizeof(T);
		if (neededSize > allocationSize) {
			// Need to replace the old allocation.
			this->impl_free();
			this->impl_allocate(elementCount);
		} else {
			// Resize the allocation within the available space.
			heap_setUsedSize(this->impl_elements, neededSize);
			this->impl_elementCount = elementCount;
		}
	}
	void impl_destroy() {
		for (intptr_t index = 0; index < this->impl_elementCount; index++) {
			this->impl_elements[index].~T();
		}
	}
public:
	// Constructors.
	Array() : impl_elementCount(0), impl_elements(nullptr) {}
	Array(const intptr_t newLength, const T& defaultValue) {
		if (newLength > 0) {
			this->impl_allocate(newLength);
			for (intptr_t index = 0; index < newLength; index++) {
				new (this->impl_elements + index) T(defaultValue);
			}
		} else {
			this->impl_elementCount = 0;
		}
	}
	// Copy constructor.
	Array(const Array<T>& source) {
		this->impl_allocate(source.impl_elementCount);
		for (intptr_t e = 0; e < this->impl_elementCount; e++) {
			new (this->impl_elements + e) T(source.impl_elements[e]);
		}
	};
	// Move constructor.
	Array(Array<T> &&source) noexcept
	: impl_elementCount(source.impl_elementCount), impl_elements(source.impl_elements) {
		source.impl_elementCount = 0;
		source.impl_elements = nullptr;
	}
	// Copy assignment.
	Array<T>& operator = (const Array<T>& source) {
		if (this != &source) {
			this->impl_destroy();
			this->impl_reallocate(source.impl_elementCount);
			// Copy impl_elements from source.
			for (intptr_t e = 0; e < this->impl_elementCount; e++) {
				new (this->impl_elements + e) T(source.impl_elements[e]);
			}
		}
		return *this;
	};
	// Move assignment.
	Array<T>& operator = (Array<T> &&source) {
		if (this != &source) {
			this->impl_destroy();
			this->impl_free();
			this->impl_elementCount = source.impl_elementCount;
			this->impl_elements = source.impl_elements;
			source.impl_elementCount = 0;
			source.impl_elements = nullptr;
		}
		return *this;
	}
	// Destructor
	~Array() {
		this->impl_destroy();
		this->impl_free();
	}
	// Bound check
	inline bool inside(intptr_t index) const {
		return 0 <= index && index < this->impl_elementCount;
	}
	inline T& unsafe_writeAccess(intptr_t index) {
		assert(this->inside(index));
		return this->impl_elements[index];
	}
	inline const T& unsafe_readAccess(intptr_t index) const {
		assert(this->inside(index));
		return this->impl_elements[index];
	}
	// Element access
	T& operator[] (const intptr_t index) {
		impl_baseZeroBoundCheck(index, this->length(), U"Array index");
		return this->impl_elements[index];
	}
	const T& operator[] (const intptr_t index) const {
		impl_baseZeroBoundCheck(index, this->length(), U"Array index");
		return this->impl_elements[index];
	}
	inline intptr_t length() const {
		return this->impl_elementCount;
	}
};

}

#endif
