// zlib open source license
//
// Copyright (c) 2023 to 2026 David Forsgren Piuva
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

#ifndef DFPSR_API_FIELD
#define DFPSR_API_FIELD

#include "../collection/Field.h"

namespace dsr {

// Returns true iff a and b have the same dimensions and content according to T's equality operator.
template<typename T>
bool operator==(const Field<T>& a, const Field<T>& b) {
	if (a.width() != b.width()
	 || a.height() != b.height()) return false;
	for (intptr_t y = 0; y < a.height(); y++) {
		for (intptr_t x = 0; x < a.width(); x++) {
			if (!(a.unsafe_readAccess(x, y) == b.unsafe_readAccess(x, y))) return false;
		}
	}
	return true;
}

// Returns false iff a and b have the same dimensions and content according to T's equality operator.
template<typename T> bool operator!=(const Field<T>& a, const Field<T>& b) { return !(a == b); }

// Printing a generic Field of elements for easy debugging.
template<typename T>
String& string_toStreamIndented(String& target, const Field<T>& collection, const ReadableString& indentation) {
	string_append(target, indentation, U"{\n");
	intptr_t maxX = collection.width() - 1;
	intptr_t maxY = collection.height() - 1;
	for (intptr_t y = 0; y <= maxY; y++) {
		string_append(target, indentation, U"\t{\n");
		for (intptr_t x = 0; x <= maxX; x++) {
			string_toStreamIndented(target, collection.unsafe_readAccess(IVector2D(x, y)), indentation + U"\t\t");
			if (x < maxX) {
				string_append(target, U",");
			}
			string_append(target, U"\n");
		}
		string_append(target, indentation, U"\t}");
		if (y < maxY) {
			string_append(target, U",");
		}
		string_append(target, U"\n");
	}
	string_append(target, indentation, U"}");
	return target;
}

}

#endif

