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

#ifndef DFPSR_API_ALGORITHM_ARRAY
#define DFPSR_API_ALGORITHM_ARRAY

#include "../collection/Array.h"

namespace dsr {

// Returns true iff a and b are equal in length and content according to T's equality operator.
template<typename T>
bool operator==(const Array<T>& a, const Array<T>& b) {
	if (a.length() != b.length()) return false;
	for (intptr_t i = 0; i < a.length(); i++) {
		if (!(a[i] == b[i])) return false;
	}
	return true;
}

// Returns false iff a and b are equal in length and content according to T's equality operator.
template<typename T> bool operator!=(const Array<T>& a, const Array<T>& b) { return !(a == b); }

// Printing a generic Array of elements for easy debugging, using the same syntax as when printing List.
template<typename T>
String& string_toStreamIndented(String& target, const Array<T>& collection, const ReadableString& indentation) {
	string_append(target, indentation, U"{\n");
	intptr_t maxIndex = collection.length() - 1;
	for (intptr_t i = 0; i <= maxIndex; i++) {
		string_toStreamIndented(target, collection[i], indentation + U"\t");
		if (i < maxIndex) {
			string_append(target, U",");
		}
		string_append(target, U"\n");
	}
	string_append(target, indentation, U"}");
	return target;
}

}

#endif

