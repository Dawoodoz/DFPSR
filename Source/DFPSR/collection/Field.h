
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

#ifndef DFPSR_COLLECTION_FIELD
#define DFPSR_COLLECTION_FIELD

#include "collections.h"
#include "../math/IVector.h"

namespace dsr {

// A 2D version of Array with built-in support for accessing elements out of bound.
template <typename T>
class Field {
private:
	const int32_t elementWidth, elementHeight;
	T *elements = nullptr;
public:
	// Constructor
	Field(const int32_t width, const int32_t height, const T& defaultValue)
	  : elementWidth(width), elementHeight(height) {
		impl_nonZeroLengthCheck(width, "New array width");
  		impl_nonZeroLengthCheck(height, "New array height");
		int32_t size = width * height;
		this->elements = new T[size];
		for (int32_t index = 0; index < size; index++) {
			this->elements[index] = defaultValue;
		}
	}
private:
	// Direct memory access
	// Precondition: this->inside(location.x, location.y)
	T& writeAccess(const IVector2D& location) {
		return this->elements[location.x + location.y * this->elementWidth];
	}
	const T& readAccess(const IVector2D& location) const {
		return this->elements[location.x + location.y * this->elementWidth];
	}
public:
	// No implicit copies, only pass by reference
	Field(const Field&) = delete;
	Field& operator=(const Field&) = delete;
	// Destructor
	~Field() { delete[] this->elements; }
	// Bound check
	bool inside(const IVector2D& location) const {
		return location.x >= 0 && location.x < this->elementWidth && location.y >= 0 && location.y < this->elementHeight;
	}
	// Read access
	T read_border(const IVector2D& location, const T& outside) const {
		if (this->inside(location)) {
			return this->readAccess(location);
		} else {
			return outside;
		}
	}
	T read_clamp(IVector2D location) const {
		if (location.x < 0) location.x = 0;
		if (location.x >= this->elementWidth) location.x = this->elementWidth - 1;
		if (location.y < 0) location.y = 0;
		if (location.y >= this->elementHeight) location.y = this->elementHeight - 1;
		return this->readAccess(location);
	}
	// Write access
	void write_ignore(const IVector2D& location, const T& value) {
		if (this->inside(location)) {
			this->writeAccess(location) = value;
		}
	}
	int32_t width() const {
		return this->elementWidth;
	}
	int32_t height() const {
		return this->elementHeight;
	}
};

}

#endif

