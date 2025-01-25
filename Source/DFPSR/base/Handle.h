// zlib open source license
//
// Copyright (c) 2024 to 2025 David Forsgren Piuva
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

#ifndef DFPSR_HANDLE
#define DFPSR_HANDLE

#include "heap.h"
#include <utility>

enum class AllocationInitialization {
	Uninitialized, // Used when the data will be instantly overwritten.
	Zeroed,        // Used for trivial data types.
	Constructed    // Used for a few objects.
};

namespace dsr {
	template <typename T>
	class Handle {
	private:
		// The internal pointer that reference counting is added to.
		//   Must be allocated using heap_allocate, so that it can be freed using heap_free when the use count reaches zero.
		T *data = nullptr;
		#ifdef SAFE_POINTER_CHECKS
			// The identity that should match the allocation header's identity.
			uint64_t allocationIdentity = 0;
			inline void validate() const {
				if (this->data != nullptr) {
					// Heap allocations are shared with all threads, so we only need to check the identity.
					AllocationHeader *header = heap_getHeader(this->data);
					if (header->allocationIdentity != this->allocationIdentity) {
						impl_throwIdentityMismatch(header->allocationIdentity, this->allocationIdentity);
					}
				}
			}
		#endif
	public:
		// Default construct an empty handle.
		Handle() {}

		// Assigns a debug name to the handled heap allocation.
		//   Returns the handle by reference to allow call chaining:
		//     return handle_create<MyType>(some, arguments).setName("data for something specific");
		//     return buffer_create(size).setName("data for something specific");
		//   Should be trivially optimized away by the compiler in release mode.
		inline Handle<T> &setName(const char *name) {
			#ifdef SAFE_POINTER_CHECKS
				heap_setAllocationName(this->data, name);
			#endif
			return *this;
		}

		// Construct from pointer.
		//   Pre-condition: data is the data allocated with heap_allocate.
		#ifdef SAFE_POINTER_CHECKS
			Handle(T* data, uint64_t allocationIdentity) noexcept
			: data(data) {
				this->allocationIdentity = allocationIdentity;
				if (this->data != nullptr) {
					heap_increaseUseCount(this->data);
				}
				this->validate();
			}
			inline uint64_t getAllocationIdentity() const { return this->allocationIdentity; }
		#else
			Handle(T* data) noexcept
			: data(data) {
				if (this->data != nullptr) {
					heap_increaseUseCount(this->data);
				}
			}
		#endif
		// Copy constructor.
		Handle(const Handle<T> &other) noexcept
		: data(other.getUnsafe()) {
			if (this->data != nullptr) {
				heap_increaseUseCount(this->data);
			}
			#ifdef SAFE_POINTER_CHECKS
				this->allocationIdentity = other.getAllocationIdentity();
				this->validate();
			#endif
		}
		// Copy constructor with static cast.
		template <typename V>
		Handle(const Handle<V> &other) noexcept
		: data(static_cast<T*>(other.getUnsafe())) {
			if (this->data != nullptr) {
				heap_increaseUseCount(this->data);
			}
			#ifdef SAFE_POINTER_CHECKS
				this->allocationIdentity = other.getAllocationIdentity();
				this->validate();
			#endif
		}
		// Move constructor.
		Handle(Handle<T> &&other) noexcept
		: data(other.takeOwnership()) {
			#ifdef SAFE_POINTER_CHECKS
				this->allocationIdentity = other.getAllocationIdentity();
				this->validate();
			#endif
		}
		// Move constructor with static cast.
		template <typename V>
		Handle(Handle<V> &&other) noexcept
		: data(static_cast<T*>(other.takeOwnership())) {
			#ifdef SAFE_POINTER_CHECKS
				this->allocationIdentity = other.getAllocationIdentity();
				this->validate();
			#endif
		}
		// Assignment.
		Handle<T>& operator = (const Handle<T> &other) {
			#ifdef SAFE_POINTER_CHECKS
				this->validate();
				this->allocationIdentity = other.getAllocationIdentity();
			#endif
			if (this->data != other.getUnsafe()) {
				// Decrease any old use count.
				if (this->data != nullptr) {
					heap_decreaseUseCount(this->data);
				}
				this->data = other.data;
				// Increase any new use count.
				if (this->data != nullptr) {
					heap_increaseUseCount(this->data);
				}
			}
			return *this;
		}
		// Assignment with static cast.
		template <typename V>
		Handle<T>& operator = (const Handle<V> &other) {
			#ifdef SAFE_POINTER_CHECKS
				this->validate();
				this->allocationIdentity = other.getAllocationIdentity();
			#endif
			if (this->data != other.getUnsafe()) {
				// Decrease any old use count.
				if (this->data != nullptr) {
					heap_decreaseUseCount(this->data);
				}
				this->data = static_cast<T*>(other.data);
				// Increase any new use count.
				if (this->data != nullptr) {
					heap_increaseUseCount(this->data);
				}
			}
			return *this;
		}
		// Move assignment.
		Handle<T>& operator = (Handle<T> &&other) {
			T* inherited = other.takeOwnership();
			#ifdef SAFE_POINTER_CHECKS
				this->validate();
				this->allocationIdentity = other.getAllocationIdentity();
			#endif
			if (this->data != inherited) {
				// Decrease any old use count.
				if (this->data != nullptr) {
					heap_decreaseUseCount(this->data);
				}
				this->data = inherited;
			}
			return *this;
		}
		// Move assignment with static cast.
		template <typename V>
		Handle<T>& operator = (Handle<V> &&other) {
			T* inherited = static_cast<T*>(other.takeOwnership());
			#ifdef SAFE_POINTER_CHECKS
				this->validate();
				this->allocationIdentity = other.getAllocationIdentity();
			#endif
			if (this->data != inherited) {
				// Decrease any old use count.
				if (this->data != nullptr) {
					heap_decreaseUseCount(this->data);
				}
				this->data = inherited;
			}
			return *this;
		}
		// Destructor.
		~Handle() {
			if (this->data != nullptr) {
				#ifdef SAFE_POINTER_CHECKS
					this->validate();
				#endif
				heap_decreaseUseCount(this->data);
			}
		}
		// Take ownership of the returned pointer from this handle.
		inline T* takeOwnership() {
			T* result = this->data;
			this->data = nullptr;
			return result;
		}
		// Check if the handle is null, using explicit syntax to explain the code.
		inline bool isNull() const { return this->data == nullptr; }
		// Check if the handle points to anything, using explicit syntax to explain the code.
		inline bool isNotNull() const { return this->data != nullptr; }
		// Access content through the handle using the -> operator.
		inline T* operator ->() const {
			#ifdef SAFE_POINTER_CHECKS
				if (this->data == nullptr) { impl_throwNullException(); }
				this->validate();
			#endif
			return this->data;
		}
		// Returns the allocation's used size in bytes.
		inline uintptr_t getUsedSize() const {
			if (this->data == nullptr) {
				return 0;
			} else {
				return heap_getUsedSize(this->data);
			}
		}
		// Get the number of elements by dividing the total size with the element size.
		inline uintptr_t getElementCount() const {
			if (this->data == nullptr) {
				return 0;
			} else {
				// When sizeof(T) is a power of two, this unsigned integer division will be optimized into a bit shift by the compiler.
				return heap_getUsedSize(this->data) / sizeof(T);
			}
		}
		// Get a SafePointer to the data, which is used temporarity to iterate over the content with bound checks in debug mode but no overhead in release mode.
		// Alignment decides how many additional bytes of padding that should be possible to access for SIMD operations.
		template <typename V = T>
		SafePointer<V> getSafe(const char * name) const {
			if (this->data == nullptr) {
				// A null handle returns a null pointer.
				return SafePointer<V>();
			} else {
				#ifdef SAFE_POINTER_CHECKS
					AllocationHeader *header = heap_getHeader(this->data);
					return SafePointer<V>(header, this->allocationIdentity, name, (V*)this->data, heap_getPaddedSize(this->data));
				#else
					return SafePointer<V>(name, (V*)this->data);
				#endif
			}
		}
		// Get an unsafe pointer.
		inline T* getUnsafe() const {
			#ifdef SAFE_POINTER_CHECKS
				this->validate();
			#endif
			return this->data;
		}
		// Get a reference.
		inline T& getReference() const {
			#ifdef SAFE_POINTER_CHECKS
				if (this->data == nullptr) { impl_throwNullException(); }
				this->validate();
			#endif
			return *(this->data);
		}
		// Get the use count.
		inline uintptr_t getUseCount() const {
			#ifdef SAFE_POINTER_CHECKS
				this->validate();
			#endif
			return this->data ? heap_getUseCount(this->data) : 0;
		}
	};

	// Construct a new Handle<T> using the heap allocator and begin reference counting.
	// The object is aligned by DSR_MAXIMUM_ALIGNMENT.
	template<typename T, typename... ARGS>
	static Handle<T> handle_create(ARGS&&...args) {
		// Reset the memory to zero before construction, in case that something was forgotten.
		// TODO: Should debug mode set the memory to a deterministic pattern to simplify detection of uninitialized variables?
		UnsafeAllocation allocation = heap_allocate(sizeof(T), true);
		// Construction from pointer increases the allocation's use count to 1.
		#ifdef SAFE_POINTER_CHECKS
			Handle<T> result((T*)(allocation.data), allocation.header->allocationIdentity);
		#else
			Handle<T> result((T*)(allocation.data));
		#endif
		if (result.isNull()) {
			impl_throwAllocationFailure();
		} else {
			new (result.getUnsafe()) T(std::forward<ARGS>(args)...);
			if (!std::is_trivially_destructible<T>::value) {
				heap_setAllocationDestructor(result.getUnsafe(), HeapDestructor([](void *toDestroy, void *externalResource) {
					// Destroy one object.
					((T*)toDestroy)->~T();
				}));
			}
		}
		return std::move(result.setName("Nameless handle object"));
	}

	// Construct an array of objects with a shared handle pointing to the first element.
	// The first element is aligned by DSR_MAXIMUM_ALIGNMENT and the rest are following directly according to sizeof(T).
	//   This allow tight packing of data for SIMD vectorization, because aligning with a SIMD vector would be pointless if each vector only contained one useful lane.
	// Pre-condition:
	//   sizeof(T) % alignof(T) == 0
	template<typename T, typename... ARGS>
	static Handle<T> handle_createArray(AllocationInitialization initialization, uintptr_t elementCount, ARGS&&...args) {
		UnsafeAllocation allocation = heap_allocate(sizeof(T) * elementCount, initialization == AllocationInitialization::Zeroed);
		// Construction from pointer increases the allocation's use count to 1.
		#ifdef SAFE_POINTER_CHECKS
			Handle<T> result((T*)(allocation.data), allocation.header->allocationIdentity);
		#else
			Handle<T> result((T*)(allocation.data));
		#endif
		if (result.isNull()) {
			impl_throwAllocationFailure();
		} else {
			if (initialization == AllocationInitialization::Constructed) {
				for (uintptr_t i = 0; i < elementCount; i++) {
					new (result.getUnsafe() + i) T(std::forward<ARGS>(args)...);
				}
			}
			if (!std::is_trivially_destructible<T>::value) {
				heap_setAllocationDestructor(result.getUnsafe(), HeapDestructor([](void *toDestroy, void *externalResource) {
					// Calculate the number of elements from the size.
					uintptr_t elementCount = heap_getUsedSize(toDestroy) / sizeof(T);
					// Destroy each element.
					for (uintptr_t i = 0; i < elementCount; i++) {
						((T*)toDestroy)[i].~T();
					}
				}));
			}
		}
		return std::move(result.setName("Nameless handle array"));
	}

	// Dynamic casting of handles.
	//   Attempts to cast from a base class to a specific class inheriting from the old type.
	//   OLD_TYPE does not have to be stated explicitly in the call, because it is provided by oldHandle.
	//   Example:
	//     Handle<TypeB> = handle_dynamicCast<TypeB>(handle_create<TypeA>(1, 2, 3));
	//   Pre-condition:
	//     The old handle must refer to a single element or nullptr, no arrays allowed.
	//   Post-condition:
	//     Returns oldHandle dynamically casted to NEW_TYPE.
	//     Returns an empty handle if the conversion failed.
	template <typename NEW_TYPE, typename OLD_TYPE>
	Handle<NEW_TYPE> handle_dynamicCast(const Handle<OLD_TYPE> &oldHandle) {
		#ifdef SAFE_POINTER_CHECKS
			return Handle<NEW_TYPE>(dynamic_cast<NEW_TYPE*>(oldHandle.getUnsafe()), oldHandle.getAllocationIdentity());
		#else
			return Handle<NEW_TYPE>(dynamic_cast<NEW_TYPE*>(oldHandle.getUnsafe()));
		#endif
	}
}

#endif
