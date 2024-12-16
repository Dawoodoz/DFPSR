// zlib open source license
//
// Copyright (c) 2017 to 2023 David Forsgren Piuva
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

// If you get segmentation faults despite using SafePointer, then check the following.
// * Are you compiling all of your code in debug mode?
//   The release mode does not perform SafePointer checks, because it is supposed to be zero overhead by letting the compiler inline the pointers.
// * Did you create a SafePointer from a memory region that you do not have access to, expired stack memory, or a region larger than the allocation?
//   SafePointer can not know which memory is safe to call if you do not give it correct information.
//   If the pointer was created without an allocation, make sure that regionStart is nullptr and claimedSize is zero.
// * Did you deallocate the memory before using the SafePointer?
//   SafePointer can not keep the allocation alive, because that would require counting references in both debug and release.

// To stay safe when using SafePointer:
// * Compile in debug mode by habit, until it is time for profiling or relase.
//   The operating system can not detect out of bound access in stack memory or arena allocations, so it may silently corrupt the memory without being caught if safety is disabled.
// * Let the Buffer create the safe pointer for you to prevent accidentally giving the wrong size, or use the default constructor for expressing null.
//   If you only need a part of the buffer's memory, use the slice function to get a subset of the memory with bound checks on construction.
// * Either create a SafePointer when needed within the buffer's scope, or store both in the same structure.
//   This makes sure that the allocation is not freed while the pointer still exists.

#ifndef DFPSR_SAFE_POINTER
#define DFPSR_SAFE_POINTER

#include <cstring>
#include <cassert>
#include <cstdint>

// Disabled in release mode
#ifndef NDEBUG
	#define SAFE_POINTER_CHECKS
#endif

namespace dsr {

struct AllocationHeader {
	uintptr_t totalSize; // Size of both header and payload.
	#ifdef SAFE_POINTER_CHECKS
		uint64_t threadHash; // Hash of the owning thread identity for thread local memory, 0 for shared memory.
		uint64_t allocationIdentity; // Rotating identity of the allocation, to know if the memory has been freed and reused within a memory allocator.
	#endif
	// Header for freed memory.
	AllocationHeader();
	// Header for allocated memory.
	// threadLocal should be true iff the memory may not be accessed from other threads, such as virtual stack memory.
	AllocationHeader(uintptr_t totalSize, bool threadLocal);
};

#ifdef SAFE_POINTER_CHECKS
	void assertInsideSafePointer(const char* method, const char* name, const uint8_t* pointer, const uint8_t* data, const uint8_t* regionStart, const uint8_t* regionEnd, const AllocationHeader *header, uint64_t allocationIdentity, intptr_t claimedSize, intptr_t elementSize);
	void assertNonNegativeSize(intptr_t size);
#endif

template<typename T>
class SafePointer {
private:
	// A pointer from regionStart to regionEnd
	//   Mutable because only the data being pointed to is write protected in a const SafePointer
	mutable T *data;
	#ifdef SAFE_POINTER_CHECKS
		// Optional pointer to an allocation header to know if it still exists and which threads are allowed to access it.
		mutable AllocationHeader *header = nullptr;
		// The identity that should match the allocation header's identity.
		mutable uint64_t allocationIdentity = 0;
		// Points to the first accessible byte, which should have the same alignment as the data pointer.
		mutable T *regionStart;
		// Marks the end of the allowed region, pointing to the first byte that is not accessible.
		mutable T *regionEnd;
		// Pointer to an ascii literal containing the name for improving error messages for crashes in debug mode.
		mutable const char *name;
	#endif
public:
	#ifdef SAFE_POINTER_CHECKS
	SafePointer() : data(nullptr), regionStart(nullptr), regionEnd(nullptr), name("Unnamed null pointer") {}
	explicit SafePointer(const char* name) : data(nullptr), regionStart(nullptr), regionEnd(nullptr), name(name) {}
	SafePointer(const char* name, T* regionStart, intptr_t regionByteSize = sizeof(T), AllocationHeader *header = nullptr)
	: data(regionStart), regionStart(regionStart), regionEnd((T*)(((uint8_t*)regionStart) + (intptr_t)regionByteSize)), name(name), header(header) {
		assertNonNegativeSize(regionByteSize);
		// If the pointer has a header, then store the allocation's identity in the pointer.
		if (header != nullptr) {
			this->allocationIdentity = header->allocationIdentity;
		}
	}
	SafePointer(const char* name, T* regionStart, intptr_t regionByteSize, T* data, AllocationHeader *header = nullptr)
	: data(data), regionStart(regionStart), regionEnd((T*)(((uint8_t*)regionStart) + (intptr_t)regionByteSize)), name(name), header(header) {
		assertNonNegativeSize(regionByteSize);
	}
	#else
	SafePointer() : data(nullptr) {}
	explicit SafePointer(const char* name) : data(nullptr) {}
	SafePointer(const char* name, T* regionStart, intptr_t regionByteSize = sizeof(T), AllocationHeader *header = nullptr) : data(regionStart) {}
	SafePointer(const char* name, T* regionStart, intptr_t regionByteSize, T* data, AllocationHeader *header = nullptr) : data(data) {}
	#endif
public:
	#ifdef SAFE_POINTER_CHECKS
	inline void assertInside(const char* method, const T* pointer, intptr_t size = (intptr_t)sizeof(T)) const {
		assertInsideSafePointer(method, this->name, (const uint8_t*)pointer, (const uint8_t*)this->data, (const uint8_t*)this->regionStart, (const uint8_t*)this->regionEnd, this->header, this->allocationIdentity, size, sizeof(T));
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
	inline int32_t getAlignmentOffset(int32_t byteAlignment) const {
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
	inline SafePointer<T> slice(const char* name, intptr_t byteOffset, intptr_t size) {
		T *newStart = (T*)(((uint8_t*)(this->data)) + byteOffset);
		#ifdef SAFE_POINTER_CHECKS
		assertInside("getSlice", newStart, size);
		return SafePointer<T>(name, newStart, size);
		#else
		return SafePointer<T>(name, newStart);
		#endif
	}
	inline const SafePointer<T> slice(const char* name, intptr_t byteOffset, intptr_t size) const {
		T *newStart = (T*)(((uint8_t*)(this->data)) + byteOffset);
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
	inline T& operator[] (intptr_t index) {
		T* address = this->data + index;
		#ifdef SAFE_POINTER_CHECKS
		assertInside("operator[]", address);
		#endif
		return *address;
	}
	inline const T& operator[] (intptr_t index) const {
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
			this->header = source.header;
			this->allocationIdentity = source.allocationIdentity;
			this->regionStart = source.regionStart;
			this->regionEnd = source.regionEnd;
			this->name = source.name;
		#endif
		return *this;
	}
};

template <typename T, typename S>
inline void safeMemoryCopy(SafePointer<T> target, const SafePointer<S>& source, intptr_t byteSize) {
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
inline void safeMemorySet(SafePointer<T>& target, uint8_t value, intptr_t byteSize) {
	#ifdef SAFE_POINTER_CHECKS
		// Target must be in valid memory
		target.assertInside("memoryCopy (target)", target.getUnchecked(), byteSize);
	#endif
	std::memset((char*)(target.getUnchecked()), value, (size_t)byteSize);
}

}

#endif

