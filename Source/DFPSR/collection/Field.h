
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
#include "../math/LVector.h"
#include "../math/UVector.h"

namespace dsr {

// A 2D version of Array with methods for padding reads and ignoring writes that are out-of-bound.
//   If you need more speed, pack elements into a Buffer and iterate
//     over them using SafePointer with SIMD aligned stride between rows.
//   Unlike Buffer, Field is a value type, so be careful not to pass it by value unless you intend to clone its content.
template <typename T>
class Field {
private:
	int64_t elementWidth = 0;
	int64_t elementHeight = 0;
	T *elements = nullptr;
public:
	// Constructor
	Field(const int64_t width, const int64_t height, const T& defaultValue)
	  : elementWidth(width), elementHeight(height) {
		impl_nonZeroLengthCheck(width, "New array width");
  		impl_nonZeroLengthCheck(height, "New array height");
		int64_t size = width * height;
		this->elements = new T[size];
		for (int64_t index = 0; index < size; index++) {
			this->elements[index] = defaultValue;
		}
	}
	// Bound check
	bool inside(int64_t x, int64_t y) const {
		return x >= 0 && x < this->elementWidth && y >= 0 && y < this->elementHeight;
	}
	// Direct memory access where bound checks are only applied in debug mode, so access out of bound will crash.
	// Precondition: this->inside(x, y)
	T& unsafe_writeAccess(int64_t x, int64_t y) {
		assert(this->inside(x, y));
		return this->elements[x + y * this->elementWidth];
	}
	// Precondition: this->inside(x, y)
	const T& unsafe_readAccess(int64_t x, int64_t y) const {
		assert(this->inside(x, y));
		return this->elements[x + y * this->elementWidth];
	}
	// Clonable by default!
	//   Be very careful not to accidentally pass a Field by value instead of reference,
	//   otherwise your side-effects might write to a temporary copy
	//   or time is wasted to clone an Field every time you look something up.
	Field(const Field<T>& source) {
		// Allocate to the same size as source.
		int64_t newSize = source.elementWidth * source.elementHeight;
		this->elements = new T[newSize];
		this->elementWidth = source.elementWidth;
		this->elementHeight = source.elementHeight;
		// Copy elements from source.
		for (int64_t e = 0; e < newSize; e++) {
			// Assign one element at a time, so that objects can be copy constructed.
			//   If the element type T is trivial and does not require calling constructors, using safeMemoryCopy with SafePointer will be much faster than using Array<T>.
			this->elements[e] = source.elements[e];
		}
	};
	// When assigning to the field, memory can be reused when the number of elements is the same.
	Field& operator=(const Field<T>& source) {
		int64_t oldSize = this->elementWidth * this->elementHeight;
		int64_t newSize = source.elementWidth * source.elementHeight;
		// Reallocate to the same size as source if needed.
		if (oldSize != newSize) {
			if (this->elements) delete[] this->elements;
			this->elements = new T[newSize];
		}
		// Update dimensions, even if the combined allocation size is the same.
		this->elementWidth = source.elementWidth;
		this->elementHeight = source.elementHeight;
		// Copy elements from source.
		for (int64_t e = 0; e < newSize; e++) {
			// Assign one element at a time, so that objects can be copy constructed.
			//   If the element type T is trivial and does not require calling constructors, using safeMemoryCopy with SafePointer will be much faster than using Array<T>.
			this->elements[e] = source.elements[e];
		}
		return *this;
	};
	// Destructor
	~Field() { if (this->elements) delete[] this->elements; }
	// Get the element at (x, y) or the outside value when (x, y) is out-of-bound.
	T read_border(int64_t x, int64_t y, const T& outside) const {
		if (this->inside(x, y)) {
			return this->unsafe_readAccess(x, y);
		} else {
			return outside;
		}
	}
	// Get the element closest to (x, y), by clamping the coordinate to valid bounds.
	T read_clamp(int64_t x, int64_t y) const {
		if (x < 0) x = 0;
		if (x >= this->elementWidth) x = this->elementWidth - 1;
		if (y < 0) y = 0;
		if (y >= this->elementHeight) y = this->elementHeight - 1;
		return this->unsafe_readAccess(x, y);
	}
	// Write value to the element at (x, y) when inside of the bounds, ignoring the operation silently when outside.
	void write_ignore(int64_t x, int64_t y, const T& value) {
		if (this->inside(x, y)) {
			this->unsafe_writeAccess(x, y) = value;
		}
	}
	int64_t width() const {
		return this->elementWidth;
	}
	int64_t height() const {
		return this->elementHeight;
	}

	// Wrappers for access using UVector instead of separate (x, y) coordinates.
	bool inside(const UVector2D& location) const { return this->inside(location.x, location.y); }
	T& unsafe_writeAccess(const UVector2D &location) { return this->unsafe_writeAccess(location.x, location.y); }
	const T& unsafe_readAccess(const UVector2D &location) const { return this->unsafe_readAccess(location.x, location.y); }
	T read_border(const UVector2D& location, const T& outside) const { return this->read_border(location.x, location.y, outside); }
	T read_clamp(UVector2D location) const { return this->read_clamp(location.x, location.y); }
	void write_ignore(const UVector2D& location, const T& value) { this->write_ignore(location.x, location.y); }

	// Wrappers for access using IVector instead of separate (x, y) coordinates.
	bool inside(const IVector2D& location) const { return this->inside(location.x, location.y); }
	T& unsafe_writeAccess(const IVector2D &location) { return this->unsafe_writeAccess(location.x, location.y); }
	const T& unsafe_readAccess(const IVector2D &location) const { return this->unsafe_readAccess(location.x, location.y); }
	T read_border(const IVector2D& location, const T& outside) const { return this->read_border(location.x, location.y, outside); }
	T read_clamp(IVector2D location) const { return this->read_clamp(location.x, location.y); }
	void write_ignore(const IVector2D& location, const T& value) { this->write_ignore(location.x, location.y); }

	// Wrappers for access using LVector instead of separate (x, y) coordinates.
	bool inside(const LVector2D& location) const { return this->inside(location.x, location.y); }
	T& unsafe_writeAccess(const LVector2D &location) { return this->unsafe_writeAccess(location.x, location.y); }
	const T& unsafe_readAccess(const LVector2D &location) const { return this->unsafe_readAccess(location.x, location.y); }
	T read_border(const LVector2D& location, const T& outside) const { return this->read_border(location.x, location.y, outside); }
	T read_clamp(LVector2D location) const { return this->read_clamp(location.x, location.y); }
	void write_ignore(const LVector2D& location, const T& value) { this->write_ignore(location.x, location.y); }
};

}

#endif
