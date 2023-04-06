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

#ifndef DFPSR_SAFE_POINTER
#define DFPSR_SAFE_POINTER

#include <cstring>
#include <cassert>
#include <stdint.h>

// Disabled in release mode
#ifndef NDEBUG
	#define SAFE_POINTER_CHECKS
#endif

namespace dsr {

// Generic implementaions
void assertInsideSafePointer(const char* method, const char* name, const uint8_t* pointer, const uint8_t* data, const uint8_t* regionStart, const uint8_t* regionEnd, int claimedSize, int elementSize);
void assertNonNegativeSize(int size);

template<typename T>
class SafePointer {
private:
	// A pointer from regionStart to regionEnd
	//   Mutable because only the data being pointed to is write protected in a const SafePointer
	mutable T *data;
	#ifdef SAFE_POINTER_CHECKS
		mutable T *regionStart;
		mutable T *regionEnd;
		mutable const char * name;
	#endif
public:
	#ifdef SAFE_POINTER_CHECKS
	SafePointer() : data(nullptr), regionStart(nullptr), regionEnd(nullptr), name("Unnamed null pointer") {}
	explicit SafePointer(const char* name) : data(nullptr), regionStart(nullptr), regionEnd(nullptr), name(name) {}
	SafePointer(const char* name, T* regionStart, int regionByteSize = sizeof(T)) : data(regionStart), regionStart(regionStart), regionEnd((T*)(((uint8_t*)regionStart) + (intptr_t)regionByteSize)), name(name) {
		assertNonNegativeSize(regionByteSize);
	}
	SafePointer(const char* name, T* regionStart, int regionByteSize, T* data) : data(data), regionStart(regionStart), regionEnd((T*)(((uint8_t*)regionStart) + (intptr_t)regionByteSize)), name(name) {
		assertNonNegativeSize(regionByteSize);
	}
	#else
	SafePointer() : data(nullptr) {}
	explicit SafePointer(const char* name) : data(nullptr) {}
	SafePointer(const char* name, T* regionStart, int regionByteSize = sizeof(T)) : data(regionStart) {}
	SafePointer(const char* name, T* regionStart, int regionByteSize, T* data) : data(data) {}
	#endif
public:
	#ifdef SAFE_POINTER_CHECKS
	inline void assertInside(const char* method, const T* pointer, int size = (int)sizeof(T)) const {
		assertInsideSafePointer(method, this->name, (const uint8_t*)pointer, (const uint8_t*)this->data, (const uint8_t*)this->regionStart, (const uint8_t*)this->regionEnd, size, sizeof(T));
	}
	inline void assertInside(const char* method) const {
		this->assertInside(method, this->data);
	}
	#endif
public:
	// Back to unsafe pointer with a clearly visible method name as a warning
	// The same can be done by mistake using the & operator on a reference
	// p.getUnsafe() = &(*p) = &(p[0])
	inline T* getUnsafe() {
		#ifdef SAFE_POINTER_CHECKS
		this->assertInside("getUnsafe");
		#endif
		return this->data;
	}
	inline const T* getUnsafe() const {
		#ifdef SAFE_POINTER_CHECKS
		this->assertInside("getUnsafe");
		#endif
		return this->data;
	}
	// Get unsafe pointer without bound checks for implementing your own safety
	inline T* getUnchecked() {
		return this->data;
	}
	inline const T* getUnchecked() const {
		return this->data;
	}
	// Returns the pointer in modulo byteAlignment
	// Returns 0 if the pointer is aligned with byteAlignment
	inline int getAlignmentOffset(int byteAlignment) const {
		return ((uintptr_t)this->data) % byteAlignment;
	}
	inline bool isNull() const {
		return this->data == nullptr;
	}
	inline bool isNotNull() const {
		return this->data != nullptr;
	}
	// Get a new safe pointer from a sub-set of data
	//  byteOffset is which byte in the source will be index zero in the new pointer
	//  size is the new pointer's size, which may not exceed the remaining available space
	inline SafePointer<T> slice(const char* name, int byteOffset, int size) {
		T *newStart = (T*)(((uint8_t*)(this->data)) + (intptr_t)byteOffset);
		#ifdef SAFE_POINTER_CHECKS
		assertInside("getSlice", newStart, size);
		return SafePointer<T>(name, newStart, size);
		#else
		return SafePointer<T>(name, newStart);
		#endif
	}
	inline const SafePointer<T> slice(const char* name, int byteOffset, int size) const {
		T *newStart = (T*)(((uint8_t*)(this->data)) + (intptr_t)byteOffset);
		#ifdef SAFE_POINTER_CHECKS
		assertInside("getSlice", newStart, size);
		return SafePointer<T>(name, newStart, size);
		#else
		return SafePointer<T>(name, newStart);
		#endif
	}
	// Dereference
	template <typename S = T>
	inline S& get() {
		#ifdef SAFE_POINTER_CHECKS
		assertInside("get", this->data, sizeof(S));
		#endif
		return *((S*)this->data);
	}
	template <typename S = T>
	inline const S& get() const {
		#ifdef SAFE_POINTER_CHECKS
		assertInside("get", this->data, sizeof(S));
		#endif
		return *((const S*)this->data);
	}
	inline T& operator*() {
		#ifdef SAFE_POINTER_CHECKS
		assertInside("operator*");
		#endif
		return *(this->data);
	}
	inline const T& operator*() const {
		#ifdef SAFE_POINTER_CHECKS
		assertInside("operator*");
		#endif
		return *(this->data);
	}
	inline T& operator[] (int index) {
		T* address = this->data + index;
		#ifdef SAFE_POINTER_CHECKS
		assertInside("operator[]", address);
		#endif
		return *address;
	}
	inline const T& operator[] (int index) const {
		T* address = this->data + index;
		#ifdef SAFE_POINTER_CHECKS
		assertInside("operator[]", address);
		#endif
		return *address;
	}
	inline void increaseBytes(intptr_t byteOffset) const {
		this->data = (T*)(((uint8_t*)(this->data)) + byteOffset);
	}
	inline void increaseElements(intptr_t elementOffset) const {
		this->data += elementOffset;
	}
	inline SafePointer<T>& operator+=(intptr_t elementOffset) {
		this->data += elementOffset;
		return *this;
	}
	inline const SafePointer<T>& operator+=(intptr_t elementOffset) const {
		this->data += elementOffset;
		return *this;
	}
	inline SafePointer<T>& operator-=(intptr_t elementOffset) {
		this->data -= elementOffset;
		return *this;
	}
	inline const SafePointer<T>& operator-=(intptr_t elementOffset) const {
		this->data -= elementOffset;
		return *this;
	}
	inline SafePointer<T> operator+(intptr_t elementOffset) {
		SafePointer<T> result = *this;
		result += elementOffset;
		return result;
	}
	inline const SafePointer<T> operator+(intptr_t elementOffset) const {
		SafePointer<T> result = *this;
		result += elementOffset;
		return result;
	}
	inline SafePointer<T> operator-(intptr_t elementOffset) {
		SafePointer<T> result = *this;
		result -= elementOffset;
		return result;
	}
	inline const SafePointer<T> operator-(intptr_t elementOffset) const {
		SafePointer<T> result = *this;
		result -= elementOffset;
		return result;
	}
	inline const SafePointer<T>& operator=(const SafePointer<T>& source) const {
		this->data = source.data;
		#ifdef SAFE_POINTER_CHECKS
			this->regionStart = source.regionStart;
			this->regionEnd = source.regionEnd;
			this->name = source.name;
		#endif
		return *this;
	}
};

template <typename T, typename S>
inline void safeMemoryCopy(SafePointer<T> target, const SafePointer<S>& source, int64_t byteSize) {
	#ifdef SAFE_POINTER_CHECKS
		// Both target and source must be in valid memory
		target.assertInside("memoryCopy (target)", target.getUnchecked(), (size_t)byteSize);
		source.assertInside("memoryCopy (source)", source.getUnchecked(), (size_t)byteSize);
		// memcpy doesn't allow pointer aliasing
		// TODO: Make a general assertion with the same style as out of bound exceptions
		assert(((const uint8_t*)target.getUnchecked()) + byteSize <= (uint8_t*)source.getUnchecked() || ((const uint8_t*)source.getUnchecked()) + byteSize <= (uint8_t*)target.getUnchecked());
	#endif
	std::memcpy(target.getUnchecked(), source.getUnchecked(), (size_t)byteSize);
}

template <typename T>
inline void safeMemorySet(SafePointer<T>& target, uint8_t value, int64_t byteSize) {
	#ifdef SAFE_POINTER_CHECKS
		// Target must be in valid memory
		target.assertInside("memoryCopy (target)", target.getUnchecked(), byteSize);
	#endif
	std::memset((char*)(target.getUnchecked()), value, (size_t)byteSize);
}

}

#endif

