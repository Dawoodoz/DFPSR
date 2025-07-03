
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
	Array<T> impl_elements;
	intptr_t impl_elementWidth = 0;
	intptr_t impl_elementHeight = 0;
public:
	// Constructors
	Field()
	  : impl_elements(), impl_elementWidth(0), impl_elementHeight(0) {
	}
	Field(const intptr_t width, const intptr_t height, const T& defaultValue) {
		if (width > 0 && height > 0) {
			this->impl_elements = Array<T>(width * height, defaultValue);
			this->impl_elementWidth = width;
			this->impl_elementHeight = height;
		}
	}
	// Bound check
	inline bool inside(intptr_t x, intptr_t y) const {
		return x >= 0 && x < this->impl_elementWidth && y >= 0 && y < this->impl_elementHeight;
	}
	// Direct memory access where bound checks are only applied in debug mode, so access out of bound will crash.
	// Precondition: this->inside(x, y)
	inline T& unsafe_writeAccess(intptr_t x, intptr_t y) {
		assert(this->inside(x, y));
		return this->impl_elements.unsafe_writeAccess(x + y * this->impl_elementWidth);
	}
	// Precondition: this->inside(x, y)
	inline const T& unsafe_readAccess(intptr_t x, intptr_t y) const {
		assert(this->inside(x, y));
		return this->impl_elements.unsafe_readAccess(x + y * this->impl_elementWidth);
	}
	// Destructor
	//~Field() { if (this->impl_elements) delete[] this->impl_elements; }
	// Get the element at (x, y) or the outside value when (x, y) is out-of-bound.
	T read_border(intptr_t x, intptr_t y, const T& outside) const {
		if (this->inside(x, y)) {
			return this->unsafe_readAccess(x, y);
		} else {
			return outside;
		}
	}
	// Get the element closest to (x, y), by clamping the coordinate to valid bounds.
	T read_clamp(intptr_t x, intptr_t y) const {
		if (x < 0) x = 0;
		if (x >= this->impl_elementWidth) x = this->impl_elementWidth - 1;
		if (y < 0) y = 0;
		if (y >= this->impl_elementHeight) y = this->impl_elementHeight - 1;
		return this->unsafe_readAccess(x, y);
	}
	// Write value to the element at (x, y) when inside of the bounds, ignoring the operation silently when outside.
	void write_ignore(intptr_t x, intptr_t y, const T& value) {
		if (this->inside(x, y)) {
			this->unsafe_writeAccess(x, y) = value;
		}
	}
	inline intptr_t width() const {
		return this->impl_elementWidth;
	}
	inline intptr_t height() const {
		return this->impl_elementHeight;
	}

	// Wrappers for access using UVector instead of separate (x, y) coordinates.
	bool inside(const UVector2D& location) const { return this->inside(location.x, location.y); }
	T& unsafe_writeAccess(const UVector2D &location) { return this->unsafe_writeAccess(location.x, location.y); }
	const T& unsafe_readAccess(const UVector2D &location) const { return this->unsafe_readAccess(location.x, location.y); }
	T read_border(const UVector2D& location, const T& outside) const { return this->read_border(location.x, location.y, outside); }
	T read_clamp(UVector2D location) const { return this->read_clamp(location.x, location.y); }
	void write_ignore(const UVector2D& location, const T& value) { this->write_ignore(location.x, location.y, value); }

	// Wrappers for access using IVector instead of separate (x, y) coordinates.
	bool inside(const IVector2D& location) const { return this->inside(location.x, location.y); }
	T& unsafe_writeAccess(const IVector2D &location) { return this->unsafe_writeAccess(location.x, location.y); }
	const T& unsafe_readAccess(const IVector2D &location) const { return this->unsafe_readAccess(location.x, location.y); }
	T read_border(const IVector2D& location, const T& outside) const { return this->read_border(location.x, location.y, outside); }
	T read_clamp(IVector2D location) const { return this->read_clamp(location.x, location.y); }
	void write_ignore(const IVector2D& location, const T& value) { this->write_ignore(location.x, location.y, value); }

	// Wrappers for access using LVector instead of separate (x, y) coordinates.
	bool inside(const LVector2D& location) const { return this->inside(location.x, location.y); }
	T& unsafe_writeAccess(const LVector2D &location) { return this->unsafe_writeAccess(location.x, location.y); }
	const T& unsafe_readAccess(const LVector2D &location) const { return this->unsafe_readAccess(location.x, location.y); }
	T read_border(const LVector2D& location, const T& outside) const { return this->read_border(location.x, location.y, outside); }
	T read_clamp(LVector2D location) const { return this->read_clamp(location.x, location.y); }
	void write_ignore(const LVector2D& location, const T& value) { this->write_ignore(location.x, location.y, value); }
};

}

#endif
