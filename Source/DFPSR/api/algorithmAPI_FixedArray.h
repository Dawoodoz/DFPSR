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

#ifndef DFPSR_API_ALGORITHM_FIXED_ARRAY
#define DFPSR_API_ALGORITHM_FIXED_ARRAY

#include "../collection/FixedArray.h"

namespace dsr {

// Returns true iff a and b are equal in content according to T's equality operator.
// If the lengths do not match, the call will not be compiled.
template<typename T, intptr_t LENGTH>
bool operator==(const FixedArray<T, LENGTH>& a, const FixedArray<T, LENGTH>& b) {
	for (intptr_t i = 0; i < LENGTH; i++) {
		if (!(a[i] == b[i])) return false;
	}
	return true;
}

// Returns false iff a and b are equal in content according to T's equality operator.
template<typename T, intptr_t LENGTH> bool operator!=(const FixedArray<T, LENGTH>& a, const FixedArray<T, LENGTH>& b) { return !(a == b); }

// Printing a generic FixedArray of elements for easy debugging, using the same syntax as when printing List.
template<typename T, intptr_t LENGTH>
String& string_toStreamIndented(String& target, const FixedArray<T, LENGTH>& collection, const ReadableString& indentation) {
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

