
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

#include <stdint.h>
#include <vector>

namespace dsr {

// Inlined Boundchecks.h
void nonZeroLengthCheck(int64_t length, const char* property);
void baseZeroBoundCheck(int64_t index, int64_t length, const char* property);

// An array list for constant time random access to elements in a LIFO stack.
// Technically, there's nothing wrong with the internals of std::vector, but its interface is horrible.
//   * Forced use of iterators for cloning and element removal is both overly complex and bloating the code.
//   * Unsigned indices will either force dangerous casting from signed, or prevent
//     the ability to loop backwards without crashing when the x < 0u criteria cannot be met.
template <typename T>
class List {
private:
	std::vector<T> backend;
public:
	// Constructor
	List() {}
	// Clonable
	// TODO: Make an optional performance warning
	List(const List& source) : backend(std::vector<T>(source.backend.begin(), source.backend.end())) {}
	int64_t length() const {
		return (int64_t)this->backend.size();
	}
	// Element access
	//   Warning! Do not push more elements to the list while a reference is being used
	T& operator[] (int64_t index) {
		baseZeroBoundCheck(index, this->length(), "List index");
		return this->backend[index];
	}
	const T& operator[] (int64_t index) const {
		baseZeroBoundCheck(index, this->length(), "List index");
		return this->backend[index];
	}
	T& first() {
		nonZeroLengthCheck(this->length(), "Length");
		return this->backend[0];
	}
	const T& first() const {
		nonZeroLengthCheck(this->length(), "Length");
		return this->backend[0];
	}
	T& last() {
		nonZeroLengthCheck(this->length(), "Length");
		return this->backend[this->length() - 1];
	}
	const T& last() const {
		nonZeroLengthCheck(this->length(), "Length");
		return this->backend[this->length() - 1];
	}
	void clear() {
		this->backend.clear();
	}
	void reserve(int64_t minimumLength) {
		this->backend.reserve(minimumLength);
	}
	// Warning! Reallocation may invalidate pointers and references to elements in the replaced buffer
	T& push(const T& newValue) {
		// Optimize for speed by assuming that we have enough memory
		if (this->length() == 0) {
			this->backend.reserve(32);
		} else if (this->length() >= (int64_t)this->backend.capacity()) {
			this->backend.reserve((int64_t)this->backend.capacity() * 4);
		}
		this->backend.push_back(newValue);
		return this->last();
	}
	template<typename... ARGS>
	T& pushConstruct(ARGS... args) {
		this->backend.emplace_back(args...);
		return this->last();
	}
	void remove(int64_t removedIndex) {
		this->backend.erase(this->backend.begin() + removedIndex);
	}
	void pop() {
		this->backend.pop_back();
	}
};

}

#endif

