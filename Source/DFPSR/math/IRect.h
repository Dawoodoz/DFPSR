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

#ifndef DFPSR_GEOMETRY_IRECT
#define DFPSR_GEOMETRY_IRECT

#include <stdint.h>
#include <math.h>
#include <algorithm>
#include "IVector.h"

namespace dsr {

class IRect {
private:
	int32_t l, t, w, h;
public:
	IRect() : l(0), t(0), w(0), h(0) {}
	IRect(int32_t left, int32_t top, int32_t width, int32_t height) : l(left), t(top), w(width), h(height) {}
public:
	int32_t left() const { return this->l; }
	int32_t top() const { return this->t; }
	int32_t width() const { return this->w; }
	int32_t height() const { return this->h; }
	int32_t right() const { return this->l + this->w; }
	int32_t bottom() const { return this->t + this->h; }
	IVector2D size() const { return IVector2D(this->w, this->h); }
	int32_t area() const { return this->w * this->h; }
	IVector2D upperLeft() const { return IVector2D(this->l, this->t); }
	IVector2D upperRight() const { return IVector2D(this->l + this->w, this->t); }
	IVector2D lowerLeft() const { return IVector2D(this->l, this->t + this->h); }
	IVector2D lowerRight() const { return IVector2D(this->l + this->w, this->t + this->h); }
	bool hasArea() const { return this->w > 0 && this->h > 0; }
	IRect expanded(int units) const { return IRect(this->l - units, this->t - units, this->w + units * 2, this->h + units * 2); }
	// Returns the intersection between a and b or a rectangle that has no width nor height if overlaps(a, b) is false
	static IRect cut(const IRect &a, const IRect &b) {
		if (overlaps(a, b)) {
			int32_t leftSide = std::max(a.left(), b.left());
			int32_t topSide = std::max(a.top(), b.top());
			int32_t rightSide = std::min(a.right(), b.right());
			int32_t bottomSide = std::min(a.bottom(), b.bottom());
			return IRect(leftSide, topSide, rightSide - leftSide, bottomSide - topSide);
		} else {
			return IRect();
		}
	}
	// Returns a bounding box of the union
	static IRect merge(const IRect &a, const IRect &b) {
		int32_t leftSide = std::min(a.left(), b.left());
		int32_t topSide = std::min(a.top(), b.top());
		int32_t rightSide = std::max(a.right(), b.right());
		int32_t bottomSide = std::max(a.bottom(), b.bottom());
		return IRect(leftSide, topSide, rightSide - leftSide, bottomSide - topSide);
	}
	// Returns true iff the rectangles have an overlapping area
	// Equivalent to hasArea(a * b)
	static bool overlaps(const IRect& a, const IRect& b) {
		return a.left() < b.right() && a.right() > b.left() && a.top() < b.bottom() && a.bottom() > b.top();
	}
	// Returns true iff the rectangles touches
	static inline bool touches(const IRect& a, const IRect& b) {
		return a.left() <= b.right() && a.right() >= b.left() && a.top() <= b.bottom() && a.bottom() >= b.top();
	}
	// Create the rectangle from exclusive intervals
	static IRect FromBounds(int32_t left, int32_t top, int32_t right, int32_t bottom) {
		return IRect(left, top, right - left, bottom - top);
	}
	// Create the rectangle from a size
	static IRect FromSize(int32_t width, int32_t height) {
		return IRect(0, 0, width, height);
	}
	static IRect FromSize(IVector2D size) {
		return IRect(0, 0, size.x, size.y);
	}
};

// Move without resizing
inline IRect operator+(const IRect &old, const IVector2D &offset) {
	return IRect(old.left() + offset.x, old.top() + offset.y, old.width(), old.height());
}
inline IRect operator-(const IRect &old, const IVector2D &offset) {
	return IRect(old.left() - offset.x, old.top() - offset.y, old.width(), old.height());
}

// Scale everything around origin
inline IRect operator*(const IRect &old, int32_t scalar) {
	return IRect(old.left() * scalar, old.top() * scalar, old.width() * scalar, old.height() * scalar);
}

// Check equality
inline bool operator==(const IRect &a, const IRect &b) {
	return a.left() == b.left() && a.top() == b.top() && a.width() == b.width() && a.height() == b.height();
}
inline bool operator!=(const IRect &a, const IRect &b) {
	return !(a == b);
}

inline String& string_toStreamIndented(String& target, const IRect& source, const ReadableString& indentation) {
	string_append(target, indentation, U"(", source.left(), U",", source.top(), U",", source.width(), U",", source.height(), U")");
	return target;
}

}

#endif

