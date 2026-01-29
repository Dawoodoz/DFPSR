
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

#ifndef DFPSR_COLLECTION_LIST
#define DFPSR_COLLECTION_LIST

#include "collections.h"
#include "../base/Handle.h"
#include "../base/DsrTraits.h"
#include <cstdint>
#include <utility>

namespace dsr {

// TODO:
// * Allow storing names of lists in debug mode for better error messages, using the same setName method as used in Handle.
// * Allow getting a SafePointer to all elements for faster loops without bound checks in release mode.
//   This needs to be marked as unsafe with a warning about potential invalidation of pointers from reallocation.

// Because elements are returned by reference, the list can not know when an element is modified.
//   Therefore it must clone the entire content when assigned by value for consistent behavior during reallocation.
//   When cloning on assignment, const can be inherited from the outside for passing read only lists as const references.
template <typename T>
class List {
private:
	T *impl_elements = nullptr;
	intptr_t impl_length = 0;
	intptr_t impl_buffer_length = 0;

	// Makes sure that there is memory available for storing at least minimumAllocatedLength elements.
	void impl_reserve(intptr_t minimumAllocatedLength) {
		if (minimumAllocatedLength > this->impl_buffer_length) {
			// Create a new memory allocation.
			UnsafeAllocation newAllocation = (heap_allocate(minimumAllocatedLength * sizeof(T), true));
			#ifdef SAFE_POINTER_CHECKS
				heap_setAllocationName(newAllocation.data, "List allocation");
			#endif
			T *newElements = (T*)(newAllocation.data);
			heap_increaseUseCount(newAllocation.header);
			// Use all available space.
			uintptr_t availableSize = heap_getAllocationSize(newAllocation.header);
			heap_setUsedSize(newAllocation.header, availableSize);
			// Move the data from the old allocation to the new allocation.
			//   The compiler should automatically call a copy constructor if the move operator is deleted.
			for (intptr_t e = 0; e < this->impl_buffer_length; e++) {
				new (newElements + e) T(std::move(this->impl_elements[e]));
			}
			// Transfer ownership to the new allocation.
			heap_decreaseUseCount(this->impl_elements);
			this->impl_elements = newElements;
			this->impl_buffer_length = availableSize / sizeof(T);
		}
	}
	void impl_setLength(intptr_t newLength) {
		if (newLength < this->impl_length) {
			for (intptr_t e = newLength; e < this->impl_length; e++) {
				// In-place descruction.
				this->impl_elements[e].~T();
			}
		} else {
			this->impl_reserve(newLength);
		}
		this->impl_length = newLength;
	}
	template<typename... ARGS>
	void impl_setElements(ARGS&&... args) {
		intptr_t elementCount = sizeof...(args);
		this->impl_reserve(elementCount);
		this->impl_length = elementCount;
		intptr_t e = 0;
		(void)std::initializer_list<int>{
			(
				new (this->impl_elements + e) T(std::move(args)), e++, 0
			)...
		};
	}
public:
	// Copy constructor
	List(const List<T>& source) {
		this->impl_setLength(source.length());
		for (intptr_t e = 0; e < source.length(); e++) {
			new (this->impl_elements + e) T(source.impl_elements[e]);
		}
	}
	// Move constructor
	List(List<T>&& source) noexcept {
		// No pre-existing content when move constructing.
		this->impl_elements = source.impl_elements;
		this->impl_length = source.impl_length;
		this->impl_buffer_length = source.impl_buffer_length;
		source.impl_elements = nullptr;
		source.impl_length = 0;
		source.impl_buffer_length = 0;
	}
	// Copy assignment operator
	List& operator = (const List<T>& source) {
		if (this != &source) {
			// Delete any pre-existing content.
			this->impl_setLength(0);
			// Clone the content.
			this->impl_setLength(source.length());
			for (intptr_t e = 0; e < source.length(); e++) {
				new (this->impl_elements + e) T(source.impl_elements[e]);
			}
		}
		return *this;
	}
	// Move assignment operator
	List& operator = (List<T>&& source) {
		if (this != &source) {
			// Delete any pre-existing content when move assigning.
			this->impl_setLength(0);
			heap_decreaseUseCount(this->impl_elements);
			// Move the content.
			this->impl_elements = source.impl_elements;
			this->impl_length = source.impl_length;
			this->impl_buffer_length = source.impl_buffer_length;
			source.impl_elements = nullptr;
			source.impl_length = 0;
			source.impl_buffer_length = 0;
		}
		return *this;
	}
	// Constructors
	List() {}
	template<
	  typename FIRST,
	  typename... OTHERS,
	  DSR_ENABLE_IF(DSR_CONVERTIBLE_TO(FIRST, T))
	>
	List(FIRST first, OTHERS&&... others) {
		this->impl_setElements(first, others...);
	}
	~List() {
		// Destroy all elements.
		this->impl_setLength(0);
		// Free the allocation.
		heap_decreaseUseCount(this->impl_elements);
		this->impl_elements = nullptr;
		this->impl_buffer_length = 0;
	}
	// Post-condition: Returns the element at index from the range 0..length-1.
	T& operator[] (intptr_t index) {
		impl_baseZeroBoundCheck(index, this->impl_length, "List index");
		return this->impl_elements[index];
	}
	const T& operator[] (intptr_t index) const {
		impl_baseZeroBoundCheck(index, this->impl_length, "List index");
		return this->impl_elements[index];
	}
	// Post-condition: Returns the number of elements in the array list.
	inline intptr_t length() const {
		return this->impl_length;
	}
	// Side-effect: Makes sure that the buffer have room for at least minimumLength elements.
	//   Warning! Reallocation may invalidate old pointers and references to elements in the replaced buffer.
	void reserve(intptr_t minimumLength) {
		impl_reserve(minimumLength);
	}
	// Post-condition: Returns an index to the first element, which is always zero.
	// Can be used for improving readability when used together with lastIndex.
	intptr_t firstIndex() const { return 0; }
	// Post-condition: Returns an index to the last element.
	intptr_t lastIndex() const { return this->impl_length - 1; }
	// Post-condition: Returns a reference to the first element.
	T& first() {
		impl_nonZeroLengthCheck(this->impl_length, "Length");
		return this->impl_elements[0];
	}
	// Post-condition: Returns a reference to the first element.
	const T& first() const {
		impl_nonZeroLengthCheck(this->impl_length, "Length");
		return this->impl_elements[0];
	}
	// Post-condition: Returns a reference to the last element.
	T& last() {
		impl_nonZeroLengthCheck(this->impl_length, "Length");
		return this->impl_elements[this->lastIndex()];
	}
	// Post-condition: Returns a reference to the last element.
	const T& last() const {
		impl_nonZeroLengthCheck(this->impl_length, "Length");
		return this->impl_elements[this->lastIndex()];
	}
	// Side-effect: Removes all elements by setting the count to zero.
	void clear() {
		this->impl_setLength(0);
	}
	// Side-effect: Swap the order of two elements.
	//   Useful for moving and sorting elements.
	void swap(intptr_t indexA, intptr_t indexB) {
		impl_baseZeroBoundCheck(indexA, this->impl_length, "Swap index A");
		impl_baseZeroBoundCheck(indexB, this->impl_length, "Swap index B");
		std::swap(this->impl_elements[indexA], this->impl_elements[indexB]);
	}
	T& push(const T& newValue) {
		this->impl_setLength(this->impl_length + 1);
		// Copy construction.
		new (&(this->last())) T(newValue);
		return this->last();
	}
	// Side-effect: Pushes a new element at the end.
	//   Warning! Reallocation may invalidate old pointers and references to elements in the replaced buffer.
	// Post-condition: Returns an index to the new element in the list.
	intptr_t pushGetIndex(const T& newValue) {
		this->impl_setLength(this->impl_length + 1);
		// Copy construction.
		new (&(this->last())) T(newValue);
		return this->lastIndex();
	}
	// Side-effect: Pushes a new element constructed using the given arguments.
	//   Warning! Reallocation may invalidate old pointers and references to elements in the replaced buffer.
	//   Warning! Do not pass an element in the list as an argument to the constructor,
	//     because reallocating will move the data from that location before being sent to the constructor.
	// Post-condition: Returns a reference to the new element in the list.
	template<typename... ARGS>
	T& pushConstruct(ARGS&&... args) {
		this->impl_setLength(this->impl_length + 1);
		// Copy construction.
		new (&(this->last())) T(std::forward<ARGS>(args)...);
		return this->last();
	}
	// Side-effect: Pushes a new element constructed using the given arguments.
	//   Warning! Reallocation may invalidate old pointers and references to elements in the replaced buffer.
	//   Warning! Do not pass an element in the list as an argument to the constructor,
	//     because reallocating will move the data from that location before being sent to the constructor.
	// Post-condition: Returns an index to the new element in the list.
	template<typename... ARGS>
	intptr_t pushConstructGetIndex(ARGS&&... args) {
		this->impl_setLength(this->impl_length + 1);
		new (&(this->last())) T(std::forward<ARGS>(args)...);
		return this->lastIndex();
	}
	// TODO: Implement constant time remove with swap for not preserving any order.
	// Side-effect: Deletes the element at removedIndex without changing the order of other elements.
	// Pre-condition: Returns true on success and false on failure.
	bool remove(intptr_t removedIndex) {
		if (0 <= removedIndex && removedIndex < this->impl_length) {
			// Shift all following elements without cloning, which will call the destructor for the removed element.
			for (intptr_t e = removedIndex; e < this->impl_length - 1; e++) {
				// Move assignment.
				this->impl_elements[e] = std::move(this->impl_elements[e + 1]);
			}
			this->impl_length--;
			return true;
		} else {
			return false;
		}
	}
	// Side-effect: Deletes the last element.
	// Pre-condition: Returns true on success and false on failure.
	bool pop() {
		if (this->impl_length > 0) {
			//impl_elements[this->impl_length - 1].~T();
			this->impl_setLength(this->impl_length - 1);
			return true;
		} else {
			return false;
		}
	}
};

}

#endif
