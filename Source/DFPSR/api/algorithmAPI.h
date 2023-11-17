// zlib open source license
//
// Copyright (c) 2023 David Forsgren Piuva
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

#ifndef DFPSR_API_ALGORITHM
#define DFPSR_API_ALGORITHM

// TODO: Split Algorithm into smaller APIs for specific types and only have the methods that need more than one collection type in the exposed API.
//       This allow using the algorithm API for a specific type without bloating the dependencies with all the types and their functions.
#include "../collection/List.h"
#include "../collection/Array.h"
#include "../collection/Field.h"

namespace dsr {

// Returns true iff a and b are equal in length and content according to T's equality operator.
template<typename T>
bool operator==(const List<T>& a, const List<T>& b) {
	if (a.length() != b.length()) return false;
	for (int64_t i = 0; i < a.length(); i++) {
		if (!(a[i] == b[i])) return false;
	}
	return true;
}

// Returns true iff a and b are equal in length and content according to T's equality operator.
template<typename T>
bool operator==(const Array<T>& a, const Array<T>& b) {
	if (a.length() != b.length()) return false;
	for (int64_t i = 0; i < a.length(); i++) {
		if (!(a[i] == b[i])) return false;
	}
	return true;
}

// Returns true iff a and b have the same dimensions and content according to T's equality operator.
template<typename T>
bool operator==(const Field<T>& a, const Field<T>& b) {
	if (a.width() != b.width()
	 || a.height() != b.height()) return false;
	for (int64_t y = 0; y < a.height(); y++) {
		for (int64_t x = 0; x < a.width(); x++) {
			if (!(a.unsafe_readAccess(IVector2D(x, y)) == b.unsafe_readAccess(IVector2D(x, y)))) return false;
		}
	}
	return true;
}

// Returns false iff a and b are equal in length and content according to T's equality operator.
template<typename T> bool operator!=(const List<T>& a, const List<T>& b) { return !(a == b); }

// Returns false iff a and b are equal in length and content according to T's equality operator.
template<typename T> bool operator!=(const Array<T>& a, const Array<T>& b) { return !(a == b); }

// Returns false iff a and b have the same dimensions and content according to T's equality operator.
template<typename T> bool operator!=(const Field<T>& a, const Field<T>& b) { return !(a == b); }

// Internal helper function
template<typename T>
String& print_collection_1D_multiline(String& target, const T& collection, const ReadableString& indentation) {
	string_append(target, indentation, U"{\n");
	int64_t maxIndex = collection.length() - 1;
	for (int64_t i = 0; i <= maxIndex; i++) {
		string_toStreamIndented(target, collection[i], indentation + "\t");
		if (i < maxIndex) {
			string_append(target, U",");
		}
		string_append(target, U"\n");
	}
	string_append(target, indentation, U"}");
	return target;
}

// Printing a generic List of elements for easy debugging.
//   A new line is used after each element, because the element type might print using multiple lines and the list might be very long.
//   No new line at the end, because the caller might want to add a comma before breaking the line.
//   If string_toStreamIndented is defined for lists of a specific element type, this template function will be overridden by the more specific function.
template<typename T>
String& string_toStreamIndented(String& target, const List<T>& collection, const ReadableString& indentation) {
	return print_collection_1D_multiline(target, collection, indentation);
}

// Printing a generic Array of elements for easy debugging, using the same syntax as when printing List.
template<typename T>
String& string_toStreamIndented(String& target, const Array<T>& collection, const ReadableString& indentation) {
	return print_collection_1D_multiline(target, collection, indentation);
}

// Printing a generic Field of elements for easy debugging.
template<typename T>
String& string_toStreamIndented(String& target, const Field<T>& collection, const ReadableString& indentation) {
	string_append(target, indentation, U"{\n");
	int64_t maxX = collection.width() - 1;
	int64_t maxY = collection.height() - 1;
	for (int64_t y = 0; y <= maxY; y++) {
		string_append(target, indentation, U"\t{");
		for (int64_t x = 0; x <= maxX; x++) {
			string_toStreamIndented(target, collection.unsafe_readAccess(IVector2D(x, y)), indentation + "\t\t");
			if (x < maxX) {
				string_append(target, U",");
			}
			string_append(target, U"\n");
		}
		if (y < maxY) {
			// Comma separate rows on a separate line, to avoid having too much indentation.
			string_append(target, U"\n", indentation, U"\t,");
		}
		string_append(target, U"\n", indentation, U"\t{\n");
	}
	string_append(target, indentation, U"}");
	return target;
}

// TODO: Implement functional sort, concatenation, union, search and conversion algorithms for List, Array and Field.
//       in a separate "algorithm" API that does not have to be exposed in headers when using the collection types.

}

#endif

