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

// Post-condition: Returns the result of function f from each element of input.
template <typename OUTPUT_TYPE, typename INPUT_TYPE>
List<OUTPUT_TYPE> list_map(const List<INPUT_TYPE> &input, const TemporaryCallback<OUTPUT_TYPE(const INPUT_TYPE &element)> &f) {
	List<OUTPUT_TYPE> result;
	result.reserve(input.length());
	if (f.hasClosure()) {
		// Optimized loop for calling lambda.
		for (intptr_t e = 0; e < input.length(); e++) {
			result.push(f.callWithClosure(input[e]));
		}
	} else {
		// Optimized loop for calling function pointer.
		for (intptr_t e = 0; e < input.length(); e++) {
			result.push(f.callWithoutClosure(input[e]));
		}
	}
	return result;
}

// Side-effects:
//   * Execute action on each element in list that satisfies condition.
// Let BACKWARD be false to loop forwards from 0 to length - 1.
// Let BACKWARD be true to loop backwards from length - 1 to 0.
// condition should return true iff the given element should call action,
// action should return true to continue the loop or false when done.
template <typename T, bool BACKWARD = false>
static List<intptr_t> list_forAll(
  const dsr::List<T> &list,
  const TemporaryCallback<bool(intptr_t index, const T &element)> &condition,
  const TemporaryCallback<bool(intptr_t index, const T &element)> &action
) {
	List<intptr_t> result;
	if (BACKWARD) {
		if (action.hasClosure()) {
			if (condition.hasClosure()) {
				// Optimized loop for testing lambda condition.
				for (intptr_t e = list.length() - 1; e >= 0; e--) {
					if (condition.callWithClosure(e, list[e])) {
						if (!action.callWithClosure(e, list[e])) break;
					}
				}
			} else {
				// Optimized loop for testing function pointer condition.
				for (intptr_t e = list.length() - 1; e >= 0; e--) {
					if (condition.callWithoutClosure(e, list[e])) {
						if (!action.callWithClosure(e, list[e])) break;
					}
				}
			}
		} else {
			if (condition.hasClosure()) {
				// Optimized loop for testing lambda condition.
				for (intptr_t e = list.length() - 1; e >= 0; e--) {
					if (condition.callWithClosure(e, list[e])) {
						if (!action.callWithoutClosure(e, list[e])) break;
					}
				}
			} else {
				// Optimized loop for testing function pointer condition.
				for (intptr_t e = list.length() - 1; e >= 0; e--) {
					if (condition.callWithoutClosure(e, list[e])) {
						if (!action.callWithoutClosure(e, list[e])) break;
					}
				}
			}
		}
	} else {
		if (action.hasClosure()) {
			if (condition.hasClosure()) {
				// Optimized loop for testing lambda condition.
				for (intptr_t e = 0; e < list.length(); e++) {
					if (condition.callWithClosure(e, list[e])) {
						if (!action.callWithClosure(e, list[e])) break;
					}
				}
			} else {
				// Optimized loop for testing function pointer condition.
				for (intptr_t e = 0; e < list.length(); e++) {
					if (condition.callWithoutClosure(e, list[e])) {
						if (!action.callWithClosure(e, list[e])) break;
					}
				}
			}
		} else {
			if (condition.hasClosure()) {
				// Optimized loop for testing lambda condition.
				for (intptr_t e = 0; e < list.length(); e++) {
					if (condition.callWithClosure(e, list[e])) {
						if (!action.callWithoutClosure(e, list[e])) break;
					}
				}
			} else {
				// Optimized loop for testing function pointer condition.
				for (intptr_t e = 0; e < list.length(); e++) {
					if (condition.callWithoutClosure(e, list[e])) {
						if (!action.callWithoutClosure(e, list[e])) break;
					}
				}
			}
		}
	}
	return result;
}

// Post-conditions:
//   * Returns a list of indices to all elements in list where condition returns true, or an empty list if the condition returned false for all elements.
template <typename T>
static inline List<intptr_t> list_findAll(const dsr::List<T> &list, const TemporaryCallback<bool(intptr_t index, const T &element)> &condition) {
	List<intptr_t> result;
	// Ascending loop
	list_forAll<T, false>(list, condition, [&result](intptr_t index, const T &element) -> bool {
		result.push(index); // Push the element's index to the list.
		return true; // Keep iterating over the list.
	});
	return result;
}

// Post-conditions:
//   * Returns a list of indices to all elements in list matching, or an empty list if there is no match.
template <typename T>
static inline List<intptr_t> list_findAll(const dsr::List<T> &list, const T &find) {
	return list_findAll<T>(list, [&find](intptr_t index, const T &element) -> bool {
		return element == find;
	});
}

// Post-conditions:
//   * Returns an index to the first element in list satisfying condition, or -1 if none could be found.
template <typename T>
static inline intptr_t list_findFirst(const dsr::List<T> &list, const TemporaryCallback<bool(intptr_t index, const T &element)> &condition) {
	intptr_t result = -1;
	// Ascending loop
	list_forAll<T, false>(list, condition, [&result](intptr_t index, const T &element) -> bool {
		result = index; // Take the index as the result.
		return false; // We are done iterating over the list.
	});
	return result;
}

// Post-conditions:
//   * Returns an index to the first element in list matching find according to T's == operator, or -1 if none could be found.
template <typename T>
static inline intptr_t list_findFirst(const dsr::List<T> &list, const T &find) {
	return list_findFirst<T>(list, [&find](intptr_t index, const T &element) -> bool {
		return element == find;
	});
}

// Post-conditions:
//   * Returns an index to the last element in list satisfying condition, or -1 if none could be found.
template <typename T>
static inline intptr_t list_findLast(const dsr::List<T> &list, const TemporaryCallback<bool(intptr_t index, const T &element)> &condition) {
	intptr_t result = -1;
	// Descending loop
	list_forAll<T, true>(list, condition, [&result](intptr_t index, const T &element) -> bool {
		result = index; // Take the index as the result.
		return false; // We are done iterating over the list.
	});
	return result;
}

// Post-conditions:
//   * Returns an index to the last element in list matching find according to T's == operator, or -1 if none could be found.
template <typename T>
static inline intptr_t list_findLast(const dsr::List<T> &list, const T &find) {
	return list_findLast<T>(list, [&find](intptr_t index, const T &element) -> bool {
		return element == find;
	});
}

// Post-condition: Returns true iff condition is satisfied for any element in list.
template <typename T>
static inline bool list_elementExists(const dsr::List<T> &list, const TemporaryCallback<bool(intptr_t index, const T &element)> &condition) {
	return list_findFirst(list, condition) != -1;
}

// Post-condition: Returns true iff find matches any element in list according to T's == operator.
template <typename T>
static inline bool list_elementExists(const dsr::List<T> &list, const T &find) {
	return list_findFirst(list, find) != -1;
}

// Post-condition: Returns true iff condition is not satisfied for any element in list.
template <typename T>
static inline bool list_elementIsMissing(const dsr::List<T> &list, const TemporaryCallback<bool(intptr_t index, const T &element)> &condition) {
	return list_findFirst(list, condition) == -1;
}

// Post-condition: Returns true iff find does not exist in list according to T's == operator.
template <typename T>
static inline bool list_elementIsMissing(const dsr::List<T> &list, const T &find) {
	return list_findFirst(list, find) == -1;
}

// Post-condition: Returns true iff the list is sorted according to sortCompare, meaning that each neigoboring pair of elements in sourceList satisfies the comparison in the sortCompare function.
template <typename T>
static bool list_isSorted(const List<T>& sourceList, const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &sortCompare) {
	if (sortCompare.hasClosure()) {
		// Optimized loop for comparing with lambda,
		//   for when you have to follow indices through captured collection to see the real meaning behind an index,
		//   or just use a lambda with an empty closure for the convenience.
		for (intptr_t e = 0; e < sourceList.length() - 1; e++) {
			if (!sortCompare.callWithClosure(sourceList[e], sourceList[e + 1])) {
				// Not sorted according to sortCompare.
				return false;
			}
		}
	} else {
		// Optimized loop for comparing with function pointer,
		//   for when a comparison can be done directly on the elements.
		for (intptr_t e = 0; e < sourceList.length() - 1; e++) {
			if (!sortCompare.callWithoutClosure(sourceList[e], sourceList[e + 1])) {
				// Not sorted according to sortCompare.
				return false;
			}
		}
	}
	// Sorted according to sortCompare.
	return true;
}

// If you have a default way of sorting T, you can just define comparison operators for the type to avoid referring to a custom comparison function every time.
// Post-condition: Returns true iff the list is sorted in ascending order according to T's <= operator, meaning that sourceList[n] <= sourceList[n + 1] for all neighboring elements.
template <typename T>
static inline bool list_isSorted_ascending(const List<T>& sourceList) {
	return list_isSorted<T>(sourceList, [](const T &leftSide, const T &rightSide) -> bool { return leftSide <= rightSide; });
}

// If you have a default way of sorting T, you can just define comparison operators for the type to avoid referring to a custom comparison function every time.
// Post-condition: Returns true iff the list is sorted in descending order according to T's >= operator, meaning that sourceList[n] >= sourceList[n + 1] for all neighboring elements.
template <typename T>
static inline bool list_isSorted_descending(const List<T>& sourceList) {
	return list_isSorted<T>(sourceList, [](const T &leftSide, const T &rightSide) -> bool { return leftSide >= rightSide; });
}

// Insert an element into an unsorted list and return the element's new index.
// For consistency with the insert_unique functions that do not always push something to refer to, there is no version returning by reference.
// Side-effect:
//   * targetList expands to include a copy of the new element at the end.
template <typename T>
static inline intptr_t list_insert_last(dsr::List<T> &targetList, const T &element) {
	return targetList.pushGetIndex(element);
}

// Insert an element into a sorted list and return its new index.
// For consistency with the insert_unique functions that do not always push something to refer to, there is no version returning by reference.
// The insertion is stable, so no pre-existing equal elements will change order between each other.
//   If targetList already contains any elements that are considered equal to element, the new element will be placed behind all of them.
// Pre-condition:
//   * targetList must be sorted according to sortCompare, so that list_isSorted(targetList, sortCompare) would return true.
// Side-effect:
//   * targetList expands to include a copy of the new element at a sorted location.
//   * targetList remains sorted according to sortCompare, so that list_isSorted(targetList, sortCompare) will still return true.
// Post-condition:
//   * Returns the inserted element's new index in targetList.
template <typename T>
static intptr_t list_insert_sorted(dsr::List<T> &targetList, const T &element, const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &sortCompare) {
	#ifndef NDEBUG
		if (!list_isSorted(targetList, sortCompare)) {
			throwError(U"Tried to call list_insert_sorted with a target list that was not already sorted!\n");
		}
	#endif
	targetList.push(element);
	intptr_t at = targetList.length() - 1;
	while (at > 0 && !sortCompare(targetList[at - 1], targetList[at])) {
		targetList.swap(at - 1, at);
		at--;
	}
	return at;
}

// Insert an element into a sorted list and return its new index.
// For consistency with the insert_unique functions that do not always push something to refer to, there is no version returning by reference.
// The insertion is stable, so no pre-existing equal elements will change order between each other.
//   If targetList already contains any elements that are considered equal to element, the new element will be placed behind all of them.
// Pre-condition:
//   targetList must be sorted according to T's <= operator, so that list_isSorted_ascending(targetList) would return true.
// Side-effect:
//   * targetList expands to include a copy of the new element at a sorted location.
//   * targetList remains sorted according to T's <= operator, so that list_isSorted_ascending(targetList) will still return true.
// Post-condition:
//   * Returns the inserted element's new index in targetList.
template <typename T>
static inline intptr_t list_insert_sorted_ascending(dsr::List<T> &targetList, const T &element) {
	return list_insert_sorted<T>(targetList, element, [](const T &leftSide, const T &rightSide) -> bool { return leftSide <= rightSide; });
}

// Insert an element into a sorted list and return its new index.
// For consistency with the insert_unique functions that do not always push something to refer to, there is no version returning by reference.
// The insertion is stable, so no pre-existing equal elements will change order between each other.
//   If targetList already contains any elements that are considered equal to element, the new element will be placed behind all of them.
// Pre-condition:
//   targetList must be sorted according to T's >= operator, so that list_isSorted_descending(targetList) would return true.
// Side-effect:
//   * targetList expands to include a copy of the new element at a sorted location.
//   * targetList remains sorted according to T's >= operator, so that list_isSorted_descending(targetList) will still return true.
// Post-condition:
//   * Returns the inserted element's new index in targetList.
template <typename T>
static inline intptr_t list_insert_sorted_descending(dsr::List<T> &targetList, const T &element) {
	return list_insert_sorted<T>(targetList, element, [](const T &leftSide, const T &rightSide) -> bool { return leftSide >= rightSide; });
}

// Insert copies of the elements from sourceList into the end of targetList.
// Side-effect:
//   * targetList expands to include a copy of all elements in sourceList.
template <typename T>
static inline void list_append_last(dsr::List<T> &targetList, const dsr::List<T> &sourceList) {
	// Reserve enough space for all the new elements.
	targetList.reserve(targetList.length() + sourceList.length());
	// Place the elements at the end.
	for (intptr_t e = 0; e < sourceList.length(); e++) {
		list_insert_last(targetList, sourceList[e]);
	}
}

// Convenient wrapper for calling list_insert_sorted for all elements in sourceList.
//   Each element is inserted individually to find its sorted location within targetList.
// Call list_insert_sorted directly if you need to know the new indices from its result.
template <typename T>
static inline void list_append_sorted(dsr::List<T> &targetList, const dsr::List<T> &sourceList, const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &sortCompare) {
	// Reserve enough space for all the new elements.
	targetList.reserve(targetList.length() + sourceList.length());
	// Insert the elements.
	for (intptr_t e = 0; e < sourceList.length(); e++) {
		list_insert_sorted<T>(targetList, sourceList[e], sortCompare);
	}
}

// Convenient wrapper for calling list_insert_sorted_ascending for all elements in sourceList.
//   Each element is inserted individually to find its sorted location within targetList.
// Call list_insert_sorted_ascending directly if you need to know the new indices from its result.
template <typename T>
static inline void list_append_sorted_ascending(dsr::List<T> &targetList, const dsr::List<T> &sourceList) {
	// Reserve enough space for all the new elements.
	targetList.reserve(targetList.length() + sourceList.length());
	// Insert the elements.
	for (intptr_t e = 0; e < sourceList.length(); e++) {
		list_insert_sorted_ascending<T>(targetList, sourceList[e]);
	}
}

// Convenient wrapper for calling list_insert_sorted_descending for all elements in sourceList.
//   Each element is inserted individually to find its sorted location within targetList.
// Call list_insert_sorted_descending directly if you need to know the new indices from its result.
template <typename T>
static inline void list_append_sorted_descending(dsr::List<T> &targetList, const dsr::List<T> &sourceList) {
	// Reserve enough space for all the new elements.
	targetList.reserve(targetList.length() + sourceList.length());
	// Insert the elements.
	for (intptr_t e = 0; e < sourceList.length(); e++) {
		list_insert_sorted_descending<T>(targetList, sourceList[e]);
	}
}

// Only inserting element when element does not have an equal in targetList.
// Pre-conditions:
//   * All elements in targetList must be unique according to compareEqual, or else they will remain duplicated.
// Side-effects:
//   * Pushes element to targetList iff the element was missing according to compareEqual and therefore inserted into targetList.
// Post-conditions:
//   * Returns true iff the element was unique and therefore inserted into targetList.
template <typename T>
static inline intptr_t list_insert_unique_last(
  dsr::List<T> &targetList,
  const T &element,
  const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &compareEqual = [](const T &leftSide, const T &rightSide) -> bool { return leftSide == rightSide; }
) {
	if (list_elementIsMissing<T>(targetList, [&element, &compareEqual](intptr_t otherElementIndex, const T &otherElement) -> bool {
		return compareEqual(element, otherElement);
	})) {
		return list_insert_last<T>(targetList, element);
	} else {
		return -1;
	}
}

// Pre-conditions:
//   * All elements in targetList must be unique according to compareEqual, or else they will remain duplicated.
//     compareEqual defaults to T's == operator if not provided.
//   * All elements in targetList must be sorted according to sortCompare, so that sortCompare(targetList[n], targetList[n + 1]) is satisfied for all neighboring pairs.
// Side-effects:
//   * Pushes element to a sorted location in targetList.
// Pre-conditions;
//   * targetList is sorted according to the < operator when beginning the call.
// Post-conditions:
//   * Returns element's new index in targetList, or -1 if it was not inserted.
//   * targetList will remain sorted if it was sorted from the start.
template <typename T>
static inline intptr_t list_insert_unique_sorted(
  dsr::List<T> &targetList,
  const T &element,
  const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &sortCompare,
  const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &compareEqual = [](const T &leftSide, const T &rightSide) -> bool { return leftSide == rightSide; }
) {
	if (list_elementIsMissing<T>(targetList, [&element, &compareEqual](intptr_t otherElementIndex, const T &otherElement) -> bool {
		return compareEqual(element, otherElement);
	})) {
		return list_insert_sorted<T>(targetList, element, sortCompare);
	} else {
		return -1;
	}
}

// Pre-conditions:
//   * All elements in targetList must be unique according to compareEqual, or else they will remain duplicated.
//     compareEqual defaults to T's == operator if not provided.
//   * All elements in targetList must be sorted according to T's <= operator, so that targetList[n] <= targetList[n + 1] is satisfied for all neighboring pairs.
// Side-effects:
//   * Pushes element to a sorted location in targetList.
// Pre-conditions;
//   * targetList is sorted according to the < operator when beginning the call.
// Post-conditions:
//   * Returns element's new index in targetList, or -1 if it was not inserted.
//   * targetList will remain sorted if it was sorted from the start.
template <typename T>
static inline intptr_t list_insert_unique_sorted_ascending(
  dsr::List<T> &targetList,
  const T &element,
  const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &compareEqual = [](const T &leftSide, const T &rightSide) -> bool { return leftSide == rightSide; }
) {
	return list_insert_unique_sorted<T>(targetList, element, [](const T &leftSide, const T &rightSide) -> bool { return leftSide <= rightSide; }, compareEqual);
}

// Pre-conditions:
//   * All elements in targetList must be unique according to compareEqual, or else they will remain duplicated.
//     compareEqual defaults to T's == operator if not provided.
//   * All elements in targetList must be sorted according to T's >= operator, so that targetList[n] >= targetList[n + 1] is satisfied for all neighboring pairs.
// Side-effects:
//   * Pushes element to a sorted location in targetList.
// Pre-conditions;
//   * targetList is sorted according to the < operator when beginning the call.
// Post-conditions:
//   * Returns element's new index in targetList, or -1 if it was not inserted.
//   * targetList will remain sorted if it was sorted from the start.
template <typename T>
static inline intptr_t list_insert_unique_sorted_descending(
  dsr::List<T> &targetList,
  const T &element,
  const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &compareEqual = [](const T &leftSide, const T &rightSide) -> bool { return leftSide == rightSide; }
) {
	return list_insert_unique_sorted<T>(targetList, element, [](const T &leftSide, const T &rightSide) -> bool { return leftSide >= rightSide; }, compareEqual);
}

// Pre-conditions:
//   * All elements in targetList must be unique, or else they will remain duplicated.
//   * targetList and sourceList may not refer to the same list.
// Pushes all elements in sourceList that does not already exist in targetList.
template <typename T>
static inline void list_append_unique_last(
  dsr::List<T> &targetList,
  const dsr::List<T> &sourceList,
  const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &compareEqual = [](const T &leftSide, const T &rightSide) -> bool { return leftSide == rightSide; }
) {
	for (intptr_t e = 0; e < sourceList.length(); e++) {
		list_insert_unique_last<T>(targetList, sourceList[e], compareEqual);
	}
}

// Pre-conditions:
//   All elements in targetList must be unique, or else they will remain duplicated.
//   targetList must be sorted in ascending order.
//   targetList and sourceList may not refer to the same list.
// Pushes all elements in sourceList that does not already exist in targetList.
// Returns true iff any element was pushed to targetList.
template <typename T>
static inline void list_append_unique_sorted_ascending(
  dsr::List<T> &targetList,
  const dsr::List<T> &sourceList,
  const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &compareEqual = [](const T &leftSide, const T &rightSide) -> bool { return leftSide == rightSide; }
) {
	for (intptr_t e = 0; e < sourceList.length(); e++) {
		list_insert_unique_sorted_ascending<T>(targetList, sourceList[e], compareEqual);
	}
}

// Pre-conditions:
//   All elements in targetList must be unique, or else they will remain duplicated.
//   targetList must be sorted in ascending order.
//   targetList and sourceList may not refer to the same list.
// Pushes all elements in sourceList that does not already exist in targetList.
// Returns true iff any element was pushed to targetList.
template <typename T>
static inline void list_append_unique_sorted_descending(
  dsr::List<T> &targetList,
  const dsr::List<T> &sourceList,
  const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &compareEqual = [](const T &leftSide, const T &rightSide) -> bool { return leftSide == rightSide; }
) {
	for (intptr_t e = 0; e < sourceList.length(); e++) {
		list_insert_unique_sorted_descending<T>(targetList, sourceList[e], compareEqual);
	}
}

// Helper function for heapSort.
template <typename T>
static void impl_list_heapify(dsr::List<T>& targetList, intptr_t n, intptr_t i, const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &sortCompare) {
	intptr_t largest = i;
	intptr_t l = 2 * i + 1;
	intptr_t r = 2 * i + 2;
	if (l < n && !sortCompare(targetList[l], targetList[largest])) {
		largest = l;
	}
	if (r < n && !sortCompare(targetList[r], targetList[largest])) {
		largest = r;
	}
	if (largest != i) {
		targetList.swap(i, largest);
		impl_list_heapify(targetList, n, largest, sortCompare);
	}
}

// Apply the heap-sort algorithm to targetList.
//   The heap-sort algorithm does not preserve a stable order between elements that are equal according to sortCompare.
// The sortCompare function should return true when leftSide and rightSide are sorted correctly.
// Pre-condition:
//   The sortCompare function must return true when leftSide and rightSide are equal, because elements in the list might be identical.
// Side-effects:
//   Overwrites the input with the result, by sorting it in-place.
//   The elements returned by reference in targetList is a permutation of the original elements,
//   where each neighboring pair of elements satisfy the sortCompare condition.
template <typename T>
static void list_heapSort(List<T>& targetList, const TemporaryCallback<bool(const T &leftSide, const T &rightSide)> &sortCompare) {
	intptr_t n = targetList.length();
	for (intptr_t i = n / 2 - 1; i >= 0; i--) {
		dsr::impl_list_heapify(targetList, n, i, sortCompare);
	}
	for (intptr_t i = n - 1; i > 0; i--) {
		targetList.swap(0, i);
		dsr::impl_list_heapify<T>(targetList, i, 0, sortCompare);
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

}

#endif

