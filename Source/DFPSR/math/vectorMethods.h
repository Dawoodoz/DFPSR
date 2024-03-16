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

#ifndef DFPSR_GEOMETRY_VECTOR_METHODS
#define DFPSR_GEOMETRY_VECTOR_METHODS

#include <cstdint>
#include <cassert>
#include <cmath>
#include "../api/stringAPI.h"

// Since using templates for vector operands may include unwanted function
// definitions that does not make any sense and will crash when called,
// these macros allow picking specific methods that make sense for the element
// type and asserting that they can all be called in all possible combinations.

#define VECTOR_BODY_2D(VECTOR_TYPE, ELEMENT_TYPE, DEFAULT_VALUE) \
ELEMENT_TYPE x, y; \
VECTOR_TYPE() : x(DEFAULT_VALUE), y(DEFAULT_VALUE) {} \
VECTOR_TYPE(ELEMENT_TYPE x, ELEMENT_TYPE y) : x(x), y(y) {} \
explicit VECTOR_TYPE(ELEMENT_TYPE s) : x(s), y(s) {} \
ELEMENT_TYPE& operator[] (int index) { \
	assert(index >= 0 || index < 2); \
	if (index <= 0) { \
		return this->x; \
	} else { \
		return this->y; \
	} \
} \
inline VECTOR_TYPE& operator+=(const VECTOR_TYPE &offset) { \
	this->x += offset.x; \
	this->y += offset.y; \
	return *this; \
} \
inline VECTOR_TYPE& operator+=(const ELEMENT_TYPE &offset) { \
	this->x += offset; \
	this->y += offset; \
	return *this; \
} \
inline VECTOR_TYPE& operator-=(const VECTOR_TYPE &offset) { \
	this->x -= offset.x; \
	this->y -= offset.y; \
	return *this; \
} \
inline VECTOR_TYPE& operator-=(const ELEMENT_TYPE &offset) { \
	this->x -= offset; \
	this->y -= offset; \
	return *this; \
}

#define VECTOR_BODY_3D(VECTOR_TYPE, ELEMENT_TYPE, DEFAULT_VALUE) \
ELEMENT_TYPE x, y, z; \
VECTOR_TYPE() : x(DEFAULT_VALUE), y(DEFAULT_VALUE), z(DEFAULT_VALUE) {} \
VECTOR_TYPE(ELEMENT_TYPE x, ELEMENT_TYPE y, ELEMENT_TYPE z) : x(x), y(y), z(z) {} \
explicit VECTOR_TYPE(ELEMENT_TYPE s) : x(s), y(s), z(s) {} \
ELEMENT_TYPE& operator[] (int index) { \
	assert(index >= 0 || index < 3); \
	if (index <= 0) { \
		return this->x; \
	} else if (index == 1) { \
		return this->y; \
	} else { \
		return this->z; \
	} \
} \
inline VECTOR_TYPE& operator+=(const VECTOR_TYPE &offset) { \
	this->x += offset.x; \
	this->y += offset.y; \
	this->z += offset.z; \
	return *this; \
} \
inline VECTOR_TYPE& operator+=(const ELEMENT_TYPE &offset) { \
	this->x += offset; \
	this->y += offset; \
	this->z += offset; \
	return *this; \
} \
inline VECTOR_TYPE& operator-=(const VECTOR_TYPE &offset) { \
	this->x -= offset.x; \
	this->y -= offset.y; \
	this->z -= offset.z; \
	return *this; \
} \
inline VECTOR_TYPE& operator-=(const ELEMENT_TYPE &offset) { \
	this->x -= offset; \
	this->y -= offset; \
	this->z -= offset; \
	return *this; \
}

#define VECTOR_BODY_4D(VECTOR_TYPE, ELEMENT_TYPE, DEFAULT_VALUE) \
ELEMENT_TYPE x, y, z, w; \
VECTOR_TYPE() : x(DEFAULT_VALUE), y(DEFAULT_VALUE), z(DEFAULT_VALUE), w(DEFAULT_VALUE) {} \
VECTOR_TYPE(ELEMENT_TYPE x, ELEMENT_TYPE y, ELEMENT_TYPE z, ELEMENT_TYPE w) : x(x), y(y), z(z), w(w) {} \
explicit VECTOR_TYPE(ELEMENT_TYPE s) : x(s), y(s), z(s), w(s) {} \
ELEMENT_TYPE& operator[] (int index) { \
	assert(index >= 0 || index < 4); \
	if (index <= 0) { \
		return this->x; \
	} else if (index == 1) { \
		return this->y; \
	} else if (index == 2) { \
		return this->z; \
	} else { \
		return this->w; \
	} \
} \
inline VECTOR_TYPE& operator+=(const VECTOR_TYPE &offset) { \
	this->x += offset.x; \
	this->y += offset.y; \
	this->z += offset.z; \
	this->w += offset.w; \
	return *this; \
} \
inline VECTOR_TYPE& operator+=(const ELEMENT_TYPE &offset) { \
	this->x += offset; \
	this->y += offset; \
	this->z += offset; \
	this->w += offset; \
	return *this; \
} \
inline VECTOR_TYPE& operator-=(const VECTOR_TYPE &offset) { \
	this->x -= offset.x; \
	this->y -= offset.y; \
	this->z -= offset.z; \
	this->w -= offset.w; \
	return *this; \
} \
inline VECTOR_TYPE& operator-=(const ELEMENT_TYPE &offset) { \
	this->x -= offset; \
	this->y -= offset; \
	this->z -= offset; \
	this->w -= offset; \
	return *this; \
}

#define OPERATORS_2D(VECTOR_TYPE, ELEMENT_TYPE) \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x + right.x, left.y + right.y); \
} \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x + right, left.y + right); \
} \
inline VECTOR_TYPE operator+(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left + right.x, left + right.y); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x - right.x, left.y - right.y); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x - right, left.y - right); \
} \
inline VECTOR_TYPE operator-(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left - right.x, left - right.y); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x * right.x, left.y * right.y); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x * right, left.y * right); \
} \
inline VECTOR_TYPE operator*(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left * right.x, left * right.y); \
} \
inline VECTOR_TYPE operator/(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x / right.x, left.y / right.y); \
} \
inline VECTOR_TYPE operator/(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x / right, left.y / right); \
} \
inline VECTOR_TYPE operator/(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left / right.x, left / right.y); \
}

#define OPERATORS_3D(VECTOR_TYPE, ELEMENT_TYPE) \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x + right.x, left.y + right.y, left.z + right.z); \
} \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x + right, left.y + right, left.z + right); \
} \
inline VECTOR_TYPE operator+(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left + right.x, left + right.y, left + right.z); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x - right.x, left.y - right.y, left.z - right.z); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x - right, left.y - right, left.z - right); \
} \
inline VECTOR_TYPE operator-(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left - right.x, left - right.y, left - right.z); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x * right.x, left.y * right.y, left.z * right.z); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x * right, left.y * right, left.z * right); \
} \
inline VECTOR_TYPE operator*(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left * right.x, left * right.y, left * right.z); \
} \
inline VECTOR_TYPE operator/(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x / right.x, left.y / right.y, left.z / right.z); \
} \
inline VECTOR_TYPE operator/(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x / right, left.y / right, left.z / right); \
} \
inline VECTOR_TYPE operator/(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left / right.x, left / right.y, left / right.z); \
}

#define OPERATORS_4D(VECTOR_TYPE, ELEMENT_TYPE) \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w); \
} \
inline VECTOR_TYPE operator+(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x + right, left.y + right, left.z + right, left.w + right); \
} \
inline VECTOR_TYPE operator+(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left + right.x, left + right.y, left + right.z, left + right.w); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w); \
} \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x - right, left.y - right, left.z - right, left.w - right); \
} \
inline VECTOR_TYPE operator-(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left - right.x, left - right.y, left - right.z, left - right.w); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x * right.x, left.y * right.y, left.z * right.z, left.w * right.w); \
} \
inline VECTOR_TYPE operator*(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x * right, left.y * right, left.z * right, left.w * right); \
} \
inline VECTOR_TYPE operator*(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left * right.x, left * right.y, left * right.z, left * right.w); \
} \
inline VECTOR_TYPE operator/(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left.x / right.x, left.y / right.y, left.z / right.z, left.w / right.w); \
} \
inline VECTOR_TYPE operator/(const VECTOR_TYPE &left, ELEMENT_TYPE right) { \
	return VECTOR_TYPE(left.x / right, left.y / right, left.z / right, left.w / right); \
} \
inline VECTOR_TYPE operator/(ELEMENT_TYPE left, const VECTOR_TYPE &right) { \
	return VECTOR_TYPE(left / right.x, left / right.y, left / right.z, left / right.w); \
}

#define SIGNED_OPERATORS_2D(VECTOR_TYPE, ELEMENT_TYPE) \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &v) { \
	return VECTOR_TYPE(-v.x, -v.y); \
}

#define SIGNED_OPERATORS_3D(VECTOR_TYPE, ELEMENT_TYPE) \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &v) { \
	return VECTOR_TYPE(-v.x, -v.y, -v.z); \
}

#define SIGNED_OPERATORS_4D(VECTOR_TYPE, ELEMENT_TYPE) \
inline VECTOR_TYPE operator-(const VECTOR_TYPE &v) { \
	return VECTOR_TYPE(-v.x, -v.y, -v.z, -v.w); \
}

#define OPPOSITE_COMPARE_2D(VECTOR_TYPE) \
inline bool operator!=(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return !(left == right); \
}

#define EXACT_COMPARE_2D(VECTOR_TYPE) \
inline bool operator==(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return left.x == right.x && left.y == right.y; \
} \
OPPOSITE_COMPARE_2D(VECTOR_TYPE)

#define EXACT_COMPARE_3D(VECTOR_TYPE) \
inline bool operator==(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return left.x == right.x && left.y == right.y && left.z == right.z; \
} \
OPPOSITE_COMPARE_2D(VECTOR_TYPE)

#define EXACT_COMPARE_4D(VECTOR_TYPE) \
inline bool operator==(const VECTOR_TYPE &left, const VECTOR_TYPE &right) { \
	return left.x == right.x && left.y == right.y && left.z == right.z && left.w == right.w; \
} \
OPPOSITE_COMPARE_2D(VECTOR_TYPE)

#define SERIALIZATION_2D(VECTOR_TYPE) \
inline String& string_toStreamIndented(String& target, const VECTOR_TYPE& source, const ReadableString& indentation) { \
	string_append(target, indentation, source.x, U", ", source.y); \
	return target; \
}

#define SERIALIZATION_3D(VECTOR_TYPE) \
inline String& string_toStreamIndented(String& target, const VECTOR_TYPE& source, const ReadableString& indentation) { \
	string_append(target, indentation, source.x, U", ", source.y, U", ", source.z); \
	return target; \
}

#define SERIALIZATION_4D(VECTOR_TYPE) \
inline String& string_toStreamIndented(String& target, const VECTOR_TYPE& source, const ReadableString& indentation) { \
	string_append(target, indentation, source.x, U", ", source.y, U", ", source.z, U", ", source.w); \
	return target; \
}

#endif
