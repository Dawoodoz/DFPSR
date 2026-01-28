
// zlib open source license
//
// Copyright (c) 2018 to 2026 David Forsgren Piuva
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

#ifndef DFPSR_COLLECTION_FIXED_ARRAY
#define DFPSR_COLLECTION_FIXED_ARRAY

#include "collections.h"

namespace dsr {

// A variation of Array that stores the data directly by value instead of using a dynamic allocation, by knowing the size in compile time.
//   Use for small arrays of fixed size, where you just want some bound checks.
//   When passing FixedArray as an argument, pass it by reference to save time when cloning the content is not desired.
//   Be careful not to use this for large collections on the stack, because that will use actual stack memory, which is limited.
template <typename T, intptr_t LENGTH>
class FixedArray {
private:
	T impl_elements[LENGTH];
public:
	// Constructors.
	FixedArray() {
		for (intptr_t index = 0; index < LENGTH; index++) {
			new (this->impl_elements + index) T();
		}
	}
	FixedArray(const T& defaultValue) {
		for (intptr_t index = 0; index < LENGTH; index++) {
			new (this->impl_elements + index) T(defaultValue);
		}
	}
	// Bound check
	inline bool inside(intptr_t index) const {
		return 0 <= index && index < LENGTH;
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
		impl_baseZeroBoundCheck(index, LENGTH, "FixedArray index");
		return this->impl_elements[index];
	}
	const T& operator[] (const intptr_t index) const {
		impl_baseZeroBoundCheck(index, LENGTH, "FixedArray index");
		return this->impl_elements[index];
	}
	inline intptr_t length() const {
		return LENGTH;
	}
};

}

#endif
