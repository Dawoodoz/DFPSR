﻿
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

#ifndef DFPSR_COLLECTION_LIST
#define DFPSR_COLLECTION_LIST

#include "collections.h"
#include <cstdint>
#include <vector>
#include <algorithm>

namespace dsr {

// TODO: Remove the std::vector dependency by reimplementing the basic features.

// A wrapper over std::vector to improve safety, readability and performance.
//   Technically, there's nothing wrong with the internals of std::vector, but its interface is horrible.
//     * std::vector will create too many small allocations unless manually told how to reserve memory in advance.
//     * Forced use of iterators for cloning and element removal is both overly complex and bloating the code.
//       Most people joining your project won't be able to read the code if using std::iterator.
//       Safer to access elements by index, or an iterating high-level function performing a lambda for each element.
//       If performance is important, then use Buffer and SafePointer instead,
//       so that you get memory bound and alignment checks for SIMD vectors.
//     * Unsigned indices will either force dangerous casting from signed, or prevent
//       the ability to loop backwards without crashing when the x < 0u criteria cannot be met.
//   Unlike Buffer, List is a value type, so be careful not to pass it by value unless you intend to clone its content.
template <typename T>
class List {
private:
	std::vector<T> backend;
	List(const std::vector<T>& backend) : backend(backend) {}
public:
	// Constructor
	List() {}
	// Clonable by default!
	//   Be very careful not to accidentally pass a List by value instead of reference,
	//   otherwise your side-effects might write to a temporary copy
	//   or time is wasted to clone a List every time you look something up.
	List(const List& source) : backend(std::vector<T>(source.backend.begin(), source.backend.end())) {}
	// Construct using one argument per element.
	template<typename... ELEMENTS>
	List(ELEMENTS... elements)
	: backend({elements...}) {}
	// Post-condition: Returns the number of elements in the array list.
	int64_t length() const {
		return (int64_t)this->backend.size();
	}
	// Post-condition: Returns the element at index from the range 0..length-1.
	T& operator[] (int64_t index) {
		impl_baseZeroBoundCheck(index, this->length(), "List index");
		return this->backend[index];
	}
	// Post-condition: Returns the write-protected element at index from the range 0..length-1.
	const T& operator[] (int64_t index) const {
		impl_baseZeroBoundCheck(index, this->length(), "List index");
		return this->backend[index];
	}
	// Post-condition: Returns an index to the first element, which is always zero.
	// Can be used for improving readability when used together with lastIndex.
	int64_t firstIndex() const { return 0; }
	// Post-condition: Returns an index to the last element.
	int64_t lastIndex() const { return this->length() - 1; }
	// Post-condition: Returns a reference to the first element.
	T& first() {
		impl_nonZeroLengthCheck(this->length(), "Length");
		return this->backend[0];
	}
	// Post-condition: Returns a reference to the first element from a write protected array list.
	const T& first() const {
		impl_nonZeroLengthCheck(this->length(), "Length");
		return this->backend[0];
	}
	// Post-condition: Returns a reference to the last element.
	T& last() {
		impl_nonZeroLengthCheck(this->length(), "Length");
		return this->backend[this->lastIndex()];
	}
	// Post-condition: Returns a reference to the last element from a write protected array list.
	const T& last() const {
		impl_nonZeroLengthCheck(this->length(), "Length");
		return this->backend[this->lastIndex()];
	}
	// Side-effect: Removes all elements by setting the count to zero.
	void clear() {
		this->backend.clear();
	}
	// Side-effect: Makes sure that the buffer have room for at least minimumLength elements.
	//   Warning! Reallocation may invalidate old pointers and references to elements in the replaced buffer.
	void reserve(int64_t minimumLength) {
		this->backend.reserve(minimumLength);
	}
	// Side-effect: Swap the order of two elements.
	//   Useful for moving and sorting elements.
	void swap(int64_t indexA, int64_t indexB) {
		impl_baseZeroBoundCheck(indexA, this->length(), "Swap index A");
		impl_baseZeroBoundCheck(indexB, this->length(), "Swap index B");
		std::swap(this->backend[indexA], this->backend[indexB]);
	}
	// Side-effect: Pushes a new element at the end.
	//   Warning! Reallocation may invalidate old pointers and references to elements in the replaced buffer.
	// Post-condition: Returns a reference to the new element in the list.
	T& push(const T& newValue) {
		// Optimize for speed by assuming that we have enough memory.
		if (this->length() == 0) {
			this->backend.reserve(32);
		} else if (this->length() >= (int64_t)this->backend.capacity()) {
			this->backend.reserve((int64_t)this->backend.capacity() * 4);
		}
		this->backend.push_back(newValue);
		return this->last();
	}
	// Side-effect: Pushes a new element at the end.
	//   Warning! Reallocation may invalidate old pointers and references to elements in the replaced buffer.
	// Post-condition: Returns an index to the new element in the list.
	int64_t pushGetIndex(const T& newValue) {
		this->push(newValue);
		return this->lastIndex();
	}
	// Side-effect: Pushes a new element constructed using the given arguments.
	//   Warning! Reallocation may invalidate old pointers and references to elements in the replaced buffer.
	// Post-condition: Returns a reference to the new element in the list.
	template<typename... ARGS>
	T& pushConstruct(ARGS&&... args) {
		// Optimize for speed by assuming that we have enough memory.
		if (this->length() == 0) {
			this->backend.reserve(32);
		} else if (this->length() >= (int64_t)this->backend.capacity()) {
			this->backend.reserve((int64_t)this->backend.capacity() * 4);
		}
		this->backend.emplace_back(args...);
		return this->last();
	}
	// Side-effect: Pushes a new element constructed using the given arguments.
	//   Warning! Reallocation may invalidate old pointers and references to elements in the replaced buffer.
	// Post-condition: Returns an index to the new element in the list.
	template<typename... ARGS>
	int64_t pushConstructGetIndex(ARGS&&... args) {
		pushConstruct(args...);
		return this->lastIndex();
	}
	// Side-effect: Deletes the element at removedIndex.
	//   We can assume that the order is stable in the STD implementation, because ListTest.cpp would catch alternative interpretations.
	void remove(int64_t removedIndex) {
		this->backend.erase(this->backend.begin() + removedIndex);
	}
	// Side-effect: Deletes the last element.
	void pop() {
		this->backend.pop_back();
	}
};

}

#endif
