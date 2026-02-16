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

#ifndef DFPSR_API_ALGORITHM_LIST
#define DFPSR_API_ALGORITHM_LIST

#include "../collection/List.h"

namespace dsr {

// Returns true iff a and b are equal in length and content according to T's equality operator.
template<typename T>
static bool operator==(const List<T>& a, const List<T>& b) {
	if (a.length() != b.length()) return false;
	for (intptr_t i = 0; i < a.length(); i++) {
		if (!(a[i] == b[i])) return false;
	}
	return true;
}

// Returns false iff a and b are equal in length and content according to T's equality operator.
template<typename T> static bool operator!=(const List<T>& a, const List<T>& b) { return !(a == b); }

// Printing a generic List of elements for easy debugging.
//   A new line is used after each element, because the element type might print using multiple lines and the list might be very long.
//   No new line at the end, because the caller might want to add a comma before breaking the line.
//   If string_toStreamIndented is defined for lists of a specific element type, this template function will be overridden by the more specific function.
template<typename T>
static String& string_toStreamIndented(String& target, const List<T>& collection, const ReadableString& indentation) {
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

template <typename OUTPUT_TYPE, typename INPUT_TYPE>
List<OUTPUT_TYPE> list_map(const List<INPUT_TYPE> &input, const TemporaryCallback<OUTPUT_TYPE(const INPUT_TYPE &element)> &f) {
	List<OUTPUT_TYPE> result;
	result.reserve(input.length());
	for (intptr_t e = 0; e < input.length(); e++) {
		result.push(f(input[e]));
	}
	return result;
}

// TODO: Implement list_find_all, returning a list with indices to matching elements, using both == and a custom condition.

// Returns an index to the first element in list matching find, or -1 if none could be found.
template <typename T>
static intptr_t list_findFirst(const dsr::List<T> &list, const T &find) {
	for (intptr_t e = 0; e < list.length(); e++) {
		if (list[e] == find) {
			return e;
		}
	}
	return -1;
}

// Returns an index to the first element in list where condition returns true, or -1 if the condition returned false for all elements.
template <typename T>
static intptr_t list_findFirst(const dsr::List<T> &list, const TemporaryCallback<bool(const T &element)> &condition) {
	for (intptr_t e = 0; e < list.length(); e++) {
		if (condition(list[e])) {
			return e;
		}
	}
	return -1;
}

// Returns an index to the last element in list matching find, or -1 if none could be found.
template <typename T>
static intptr_t list_findLast(const dsr::List<T> &list, const T &find) {
	for (intptr_t e = list.length() - 1; e >= 0; e--) {
		if (list[e] == find) {
			return e;
		}
	}
	return -1;
}

// Returns an index to the last element in list where condition returns true, or -1 if the condition returned false for all elements.
template <typename T>
static intptr_t list_findLast(const dsr::List<T> &list, const TemporaryCallback<bool(const T &element)> &condition) {
	for (intptr_t e = list.length() - 1; e >= 0; e--) {
		if (condition(list[e])) {
			return e;
		}
	}
	return -1;
}

// Returns true iff find matches any element in list.
template <typename T>
static bool list_elementExists(const dsr::List<T> &list, const T &find) {
	return list_findFirst(list, find) != -1;
}

// Returns true iff condition is satisfied for any element in list.
template <typename T>
static bool list_elementExists(const dsr::List<T> &list, const TemporaryCallback<bool(const T &element)> &condition) {
	return list_findFirst(list, condition) != -1;
}

// Returns true iff find does not exist in list.
template <typename T>
static bool list_elementIsMissing(const dsr::List<T> &list, const T &find) {
	return list_findFirst(list, find) == -1;
}

// Returns true iff condition is not satisfied for any element in list.
template <typename T>
static bool list_elementIsMissing(const dsr::List<T> &list, const TemporaryCallback<bool(const T &element)> &condition) {
	return list_findFirst(list, condition) == -1;
}

// Inserts a single element at the end of targetList.
// Just a simple wrapper over the push operation to allow keeping the style consistent.
template <typename T>
inline static void list_insert_last(dsr::List<T> &targetList, const T &element) {
	targetList.push(element);
}

// TODO: Take the sorting order as a function argument.
// TODO: Document
template <typename T>
static void list_insert_sorted_ascending(dsr::List<T> &targetList, const T &element) {
	targetList.push(element);
	intptr_t at = targetList.length() - 1;
	while (at > 0 && targetList[at] < targetList[at - 1]) {
		targetList.swap(at, at - 1);
		at--;
	}
}

// TODO: Document
template <typename T>
static void list_append_last(dsr::List<T> &targetList, const dsr::List<T> &sourceList) {
	for (intptr_t e = 0; e < sourceList.length(); e++) {
		list_insert_last(targetList, sourceList[e]);
	}
}

// TODO: Document
template <typename T>
static void list_append_sorted_ascending(dsr::List<T> &targetList, const dsr::List<T> &sourceList) {
	for (intptr_t e = 0; e < sourceList.length(); e++) {
		list_insert_sorted_ascending(targetList, sourceList[e]);
	}
}

// TODO: Replace _last and _sorted with a template argument that can be passed from list_insertUnion.
// TODO: Take a function for equality.
// Pre-conditions:
//   All elements in targetList must be unique, or else they will remain duplicated.
// Pushes element to targetList and return true iff list_elementIsMissing.
template <typename T>
static bool list_insertUnique_last(dsr::List<T> &targetList, const T &element) {
	if (list_elementIsMissing(targetList, element)) {
		targetList.push(element);
		return true;
	} else {
		return false;
	}
}

// TODO: Take functions for both equality and sorting.
// TODO: Assert that the original list is sorted in debug mode.
// Pre-conditions:
//   All elements in targetList must be unique, or else they will remain duplicated.
//   targetList must be sorted in ascending order.
// Pushes element to a sorted location in targetList and return true iff list_elementIsMissing.
// Pre-condition; targetList is sorted according to the < operator when beginning the call.
// Side-effect: targetList will remain sorted if it was sorted from the start.
template <typename T>
static bool list_insertUnique_sorted_ascending(dsr::List<T> &targetList, const T &element) {
	if (list_elementIsMissing(targetList, element)) {
		targetList.push(element);
		intptr_t at = targetList.length() - 1;
		while (at > 0 && targetList[at] < targetList[at - 1]) {
			targetList.swap(at, at - 1);
			at--;
		}
		return true;
	} else {
		return false;
	}
}

// TODO: Having insert union without append is a bit strange, so implement a basic append function as well.

// TODO: Create a varargs version starting with nothing and adding unique elements from all lists before returning by value, so that duplicates in the first list are also reduced.
// TODO: Take a function for equality.
// Pre-conditions:
//   All elements in targetList must be unique, or else they will remain duplicated.
//   targetList and sourceList may not refer to the same list.
// Pushes all elements in sourceList that does not already exist in targetList.
// Returns true iff any element was pushed to targetList.
template <typename T>
static bool list_insertUnion_last(dsr::List<T> &targetList, const dsr::List<T> &sourceList) {
	bool result = false;
	for (intptr_t e = 0; e < sourceList.length(); e++) {
		// Must store the result in a new variable to avoid lazy evaluation with side-effects.
		bool newResult = list_insertUnique_last(targetList, sourceList[e]);
		result = result || newResult;
	}
	return result;
}

// TODO: Create a varargs version starting with nothing and adding unique elements from all lists before returning by value, so that duplicates in the first list are also reduced.
// TODO: Take functions for both equality and sorting.
// TODO: Assert that the original list is sorted in debug mode.
// Pre-conditions:
//   All elements in targetList must be unique, or else they will remain duplicated.
//   targetList must be sorted in ascending order.
//   targetList and sourceList may not refer to the same list.
// Pushes all elements in sourceList that does not already exist in targetList.
// Returns true iff any element was pushed to targetList.
template <typename T>
static bool list_insertUnion_sorted_ascending(dsr::List<T> &targetList, const dsr::List<T> &sourceList) {
	bool result = false;
	for (intptr_t e = 0; e < sourceList.length(); e++) {
		// Must store the result in a new variable to avoid lazy evaluation with side-effects.
		bool newResult = list_insertUnique_sorted_ascending(targetList, sourceList[e]);
		result = result || newResult;
	}
	return result;
}

// Helper function for heapSort.
template <typename T>
static void impl_list_heapify(dsr::List<T>& targetList, intptr_t n, intptr_t i, const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &compare) {
	intptr_t largest = i;
	intptr_t l = 2 * i + 1;
	intptr_t r = 2 * i + 2;
	if (l < n && !compare(targetList[l], targetList[largest])) {
		largest = l;
	}
	if (r < n && !compare(targetList[r], targetList[largest])) {
		largest = r;
	}
	if (largest != i) {
		targetList.swap(i, largest);
		impl_list_heapify(targetList, n, largest, compare);
	}
}

// Apply the heap-sort algorithm to targetList.
// The compare function should return true when leftSide and rightSide are sorted correctly.
// Pre-condition:
//   The compare function must return true when leftSide and rightSide are equal, because elements in the list might be identical.
// Side-effects:
//   Overwrites the input with the result, by sorting it in-place.
//   The elements returned by reference in targetList is a permutation of the original elements,
//   where each neighboring pair of elements satisfy the compare condition.
template <typename T>
static void list_heapSort(List<T>& targetList, const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &compare) {
	intptr_t n = targetList.length();
	for (intptr_t i = n / 2 - 1; i >= 0; i--) {
		dsr::impl_list_heapify(targetList, n, i, compare);
	}
	for (intptr_t i = n - 1; i > 0; i--) {
		targetList.swap(0, i);
		dsr::impl_list_heapify(targetList, i, 0, compare);
	}
}

// Using the < operator in T to heap-sort in-place with ascending order.
//   Useful for basic types where you don't want to write a custom comparison.
// Side-effects:
//   Overwrites the input with the result, by sorting it in-place.
//   The elements returned by reference in targetList is a permutation of the original elements,
//   where each following element is larger or equal to the previous element.
template <typename T>
static void list_heapSort_ascending(List<T>& targetList) {
	dsr::list_heapSort<T>(targetList, [](const T &leftSide, const T &rightSide) -> bool { return leftSide <= rightSide; });
}

// Using the > operator in T to heap-sort in-place with descending order.
//   Useful for basic types where you don't want to write a custom comparison.
// Side-effects:
//   Overwrites the input with the result, by sorting it in-place.
//   The elements returned by reference in targetList is a permutation of the original elements,
//   where each following element is smaller or equal to the previous element.
template <typename T>
static void list_heapSort_descending(List<T>& targetList) {
	dsr::list_heapSort<T>(targetList, [](const T &leftSide, const T &rightSide) -> bool { return leftSide >= rightSide; });
}

// TODO: Implement list_isSorted with a custom comparison.

template <typename T>
static bool list_isSorted_ascending(const List<T>& sourceList) {
	for (intptr_t e = 0; e < sourceList.length() - 1; e++) {
		if (sourceList[e] > sourceList[e + 1]) {
			// Not sorted in ascending order.
			return false;
		}
	}
	// Sorted in ascending order.
	return true;
}

template <typename T>
static bool list_isSorted_descending(const List<T>& sourceList) {
	for (intptr_t e = 0; e < sourceList.length() - 1; e++) {
		if (sourceList[e] < sourceList[e + 1]) {
			// Not sorted in decending order.
			return false;
		}
	}
	// Sorted in decending order.
	return true;
}

}

#endif

