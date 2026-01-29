// zlib open source license
//
// Copyright (c) 2017 to 2025 David Forsgren Piuva
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

// If you get segmentation faults despite using SafePointer, make sure to compile a debug version of the program to activate safety checks.
//   In debug mode, bound checks make sure that memory access do not go a single bit outside of the allowed region.
//   The allowed region will unually include padding for SIMD vectorization.

// If SafePointer is constructed with a pointer to the allocation head and its allocation identity (when the memory is allocated by the framework), more safety checks are done in debug mode.
//   The allocation identity is a 64-bit nonce stored in both the allocation's head and SafePointer, making sure that the memory accessed has not been freed or reused for something else.
//   The 64-bit thread hash prevent access of another thread's private memory, for consistent access rights when the virtual stack may allocate in either thread local or heap memory.

#ifndef DFPSR_SAFE_POINTER
#define DFPSR_SAFE_POINTER

#include <cstring>
#include <cassert>
#include <cstdint>
#include "memory.h"
#include "DsrTraits.h"

namespace dsr {

#ifdef SAFE_POINTER_CHECKS
	void impl_assertInsideSafePointer(const char* methodName, const char* pointerName, const uint8_t* claimedStart, const uint8_t* claimedEnd, intptr_t elementSize, const uint8_t* permittedStart, const uint8_t* permittedEnd, const AllocationHeader *header, uint64_t allocationIdentity);
	void impl_assertNonNegativeSize(intptr_t size);
#endif

template<typename T>
class SafePointer {
private:
	// This pointer is the only member that will remain in release mode, ensuring zero overhead in the final release.
	T *data;
public:
	// Additional information about the pointer is included in debug mode for tighter bound checks and error messages that actually hint what the error might be.
	#ifdef SAFE_POINTER_CHECKS
		// Points to the first accessible byte, which should have the same alignment as the data pointer.
		T *permittedStart;
		// Marks the end of the allowed region, pointing to the first byte that is not accessible.
		T *permittedEnd;
		// Pointer to an ascii literal containing the name for improving error messages for crashes in debug mode.
		const char *name;
		// Optional pointer to an allocation header to know if it still exists and which threads are allowed to access it.
		AllocationHeader *header = nullptr;
		// The identity that should match the allocation header's identity.
		uint64_t allocationIdentity = 0;
	#endif
public:
	#ifdef SAFE_POINTER_CHECKS
		// Create a null pointer.
		SafePointer() : data(nullptr), permittedStart(nullptr), permittedEnd(nullptr), name("Unnamed null pointer") {}
		explicit SafePointer(const char* name) : data(nullptr), permittedStart(nullptr), permittedEnd(nullptr), name(name) {}
		SafePointer(const char* name, T* permittedStart, intptr_t permittedByteSize = sizeof(T))
		: data(permittedStart), permittedStart(permittedStart), permittedEnd((T*)(((uint8_t*)permittedStart) + (intptr_t)permittedByteSize)), name(name) {
			impl_assertNonNegativeSize(permittedByteSize);
		}
		SafePointer(const char* name, T* permittedStart, intptr_t permittedByteSize, T* data)
		: data(data), permittedStart(permittedStart), permittedEnd((T*)(((uint8_t*)permittedStart) + (intptr_t)permittedByteSize)), name(name) {
			impl_assertNonNegativeSize(permittedByteSize);
		}
		SafePointer(AllocationHeader *header, uint64_t allocationIdentity, const char* name, T* permittedStart, intptr_t permittedByteSize = sizeof(T))
		: data(permittedStart), permittedStart(permittedStart), permittedEnd((T*)(((uint8_t*)permittedStart) + (intptr_t)permittedByteSize)), name(name), header(header), allocationIdentity(allocationIdentity) {
			impl_assertNonNegativeSize(permittedByteSize);
		}
		SafePointer(AllocationHeader *header, uint64_t allocationIdentity, const char* name, T* permittedStart, intptr_t permittedByteSize, T* data)
		: data(data), permittedStart(permittedStart), permittedEnd((T*)(((uint8_t*)permittedStart) + (intptr_t)permittedByteSize)), name(name), header(header), allocationIdentity(allocationIdentity) {
			impl_assertNonNegativeSize(permittedByteSize);
		}
	#else
		SafePointer() : data(nullptr) {}
		explicit SafePointer(const char* name) : data(nullptr) {}
		SafePointer(const char* name, T* permittedStart, intptr_t permittedByteSize = sizeof(T)) : data(permittedStart) {}
		SafePointer(const char* name, T* permittedStart, intptr_t permittedByteSize, T* data) : data(data) {}
		SafePointer(AllocationHeader *header, uint64_t allocationIdentity, const char* name, T* permittedStart, intptr_t permittedByteSize = sizeof(T)) : data(permittedStart) {}
		SafePointer(AllocationHeader *header, uint64_t allocationIdentity, const char* name, T* permittedStart, intptr_t permittedByteSize, T* data) : data(data) {}
	#endif
public:
	#ifdef SAFE_POINTER_CHECKS
	inline void assertInside(const char* methodName, const T* claimedStart, intptr_t size = (intptr_t)sizeof(T)) const {
		impl_assertInsideSafePointer(methodName, this->name, (const uint8_t*)claimedStart, ((const uint8_t*)claimedStart) + size, sizeof(T), (const uint8_t*)this->permittedStart, (const uint8_t*)this->permittedEnd, this->header, this->allocationIdentity);
	}
	inline void assertInside(const char* methodName) const {
		this->assertInside(methodName, this->data);
	}
	#endif
public:
	// Back to unsafe pointer with a clearly visible method name as a warning
	// The same can be done by mistake using the & operator on a reference
	// p.getUnsafe() = &(*p) = &(p[0])
	inline T* getUnsafe() const {
		#ifdef SAFE_POINTER_CHECKS
		this->assertInside("getUnsafe");
		#endif
		return this->data;
	}
	// Get unsafe pointer without bound checks for implementing your own safety
	inline T* getUnchecked() const {
		return this->data;
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
	inline SafePointer<T> slice(const char* name, intptr_t byteOffset, intptr_t size) {
		T *newStart = (T*)(((uint8_t*)(this->data)) + byteOffset);
		#ifdef SAFE_POINTER_CHECKS
		assertInside("getSlice", newStart, size);
		return SafePointer<T>(this->header, this->allocationIdentity, name, newStart, size);
		#else
		return SafePointer<T>(name, newStart);
		#endif
	}
	// Dereference
	template <typename S = T>
	inline S& get() const {
		#ifdef SAFE_POINTER_CHECKS
		assertInside("get", this->data, sizeof(S));
		#endif
		return *((S*)this->data);
	}
	inline T& operator*() const {
		#ifdef SAFE_POINTER_CHECKS
		assertInside("operator *");
		#endif
		return *(this->data);
	}
	inline T* operator ->() const {
		#ifdef SAFE_POINTER_CHECKS
		assertInside("operator ->");
		#endif
		return this->data;
	}
	inline T& operator[] (intptr_t index) const {
		T* address = this->data + index;
		#ifdef SAFE_POINTER_CHECKS
		assertInside("operator []", address);
		#endif
		return *address;
	}
	inline SafePointer<T> &increaseBytes(intptr_t byteOffset) {
		this->data = (T*)(((uint8_t*)(this->data)) + byteOffset);
		return *this;
	}
	inline SafePointer<T> &increaseElements(intptr_t elementOffset) {
		this->data += elementOffset;
		return *this;
	}
	inline SafePointer<T>& operator+=(intptr_t elementOffset) {
		this->data += elementOffset;
		return *this;
	}
	inline SafePointer<T>& operator-=(intptr_t elementOffset) {
		this->data -= elementOffset;
		return *this;
	}
	inline SafePointer<T> operator+(intptr_t elementOffset) const {
		SafePointer<T> result = *this;
		result += elementOffset;
		return result;
	}
	inline SafePointer<T> operator-(intptr_t elementOffset) const {
		SafePointer<T> result = *this;
		result -= elementOffset;
		return result;
	}
	// Copy constructor.
	SafePointer(const SafePointer<T> &other) noexcept
	: data(other.getUnchecked()) {
		#ifdef SAFE_POINTER_CHECKS
			this->header = other.header;
			this->allocationIdentity = other.allocationIdentity;
			this->permittedStart = other.permittedStart;
			this->permittedEnd = other.permittedEnd;
			this->name = other.name;
		#endif
	}
	// Copy constructor from non-const to const.
	template <typename U, DSR_ENABLE_IF(DSR_CHECK_RELATION(DsrTrait_SameType, T, const U))>
    SafePointer(const SafePointer<U> &other) noexcept
	: data(other.getUnchecked()) {
		#ifdef SAFE_POINTER_CHECKS
			this->header = other.header;
			this->allocationIdentity = other.allocationIdentity;
			this->permittedStart = other.permittedStart;
			this->permittedEnd = other.permittedEnd;
			this->name = other.name;
		#endif
	}
	// Assignment.
	SafePointer<T>& operator = (const SafePointer<T> &other) noexcept {
		this->data = other.getUnchecked();
		#ifdef SAFE_POINTER_CHECKS
			this->header = other.header;
			this->allocationIdentity = other.allocationIdentity;
			this->permittedStart = other.permittedStart;
			this->permittedEnd = other.permittedEnd;
			this->name = other.name;
		#endif
		return *this;
	}
	// Assignment from non-const to const.
	template <typename U, DSR_ENABLE_IF(DSR_CHECK_RELATION(DsrTrait_SameType, T, const U))>
	SafePointer<T>& operator = (const SafePointer<U> &other) noexcept {
		this->data = other.getUnchecked();
		#ifdef SAFE_POINTER_CHECKS
			this->header = other.header;
			this->allocationIdentity = other.allocationIdentity;
			this->permittedStart = other.permittedStart;
			this->permittedEnd = other.permittedEnd;
			this->name = other.name;
		#endif
		return *this;
	}
};

template <typename T, typename S>
inline void safeMemoryCopy(SafePointer<T> target, SafePointer<S> source, intptr_t byteSize) {
	T *targetPointer = target.getUnchecked();
	const T *sourcePointer = source.getUnchecked();
	#ifdef SAFE_POINTER_CHECKS
		// Both target and source must be in valid memory
		target.assertInside("memoryCopy (target)", targetPointer, (size_t)byteSize);
		source.assertInside("memoryCopy (source)", sourcePointer, (size_t)byteSize);
		// memcpy doesn't allow pointer aliasing
		// TODO: Make a general assertion with the same style as out of bound exceptions
		assert(((const uint8_t*)target.getUnchecked()) + byteSize <= (uint8_t*)source.getUnchecked() || ((const uint8_t*)source.getUnchecked()) + byteSize <= (uint8_t*)target.getUnchecked());
		assert(targetPointer != nullptr);
		assert(sourcePointer != nullptr);
		assert(byteSize > 0);
	#endif
	std::memcpy(targetPointer, sourcePointer, (size_t)byteSize);
}

template <typename T>
inline void safeMemorySet(SafePointer<T> target, uint8_t value, intptr_t byteSize) {
	T *targetPointer = target.getUnchecked();
	#ifdef SAFE_POINTER_CHECKS
		// Target must be in valid memory
		target.assertInside("memoryCopy (target)", targetPointer, byteSize);
		assert(targetPointer != nullptr);
		assert(byteSize > 0);
	#endif
	std::memset(targetPointer, value, (size_t)byteSize);
}

}

#endif
