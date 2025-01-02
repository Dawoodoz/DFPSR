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
#include <type_traits>

enum class AllocationInitialization {
	Uninitialized, // Used when the data will be instantly overwritten.
	Zeroed,        // Used for trivial data types.
	Constructed    // Used for a few objects.
};

// TODO: Implement SafePointer getter.
//       Implement [] reference access by index with bound checks that are enforced in both debug and release mode.
namespace dsr {
	template <typename T>
	class Handle {
	private:
		// The internal pointer that reference counting is added to.
		//   Must be allocated using heap_allocate, so that it can be freed using heap_free when the use count reaches zero.
		T *unsafePointer = nullptr;
		#ifdef SAFE_POINTER_CHECKS
			// Pointer to an ascii literal containing the name for improving error messages for crashes in debug mode.
			const char *name = nullptr;
			// The identity that should match the allocation header's identity.
			uint64_t allocationIdentity = 0;
			inline void validate() const {
				if (this->unsafePointer != nullptr) {
					// Heap allocations are shared with all threads, so we only need to check the identity.
					AllocationHeader *header = heap_getHeader(this->unsafePointer);
					if (header->allocationIdentity != this->allocationIdentity) {
						impl_throwIdentityMismatch(header->allocationIdentity, this->allocationIdentity);
					}
				}
			}
		#endif
	public:
		// Default construct an empty handle.
		Handle() {}
		// Construct from pointer.
		//   Pre-condition: unsafePointer is the data allocated with heap_allocate.
		#ifdef SAFE_POINTER_CHECKS
			Handle(T* unsafePointer, const char *name, uint64_t allocationIdentity) noexcept
			: unsafePointer(unsafePointer) {
				this->name = name;
				this->allocationIdentity = allocationIdentity;
				if (this->unsafePointer != nullptr) {
					heap_increaseUseCount(this->unsafePointer);
				}
				this->validate();
			}
			inline const char *getName() const { return this->name; }
			inline uint64_t getAllocationIdentity() const { return this->allocationIdentity; }
		#else
			Handle(T* unsafePointer) noexcept
			: unsafePointer(unsafePointer) {
				if (this->unsafePointer != nullptr) {
					heap_increaseUseCount(this->unsafePointer);
				}
			}
		#endif
		// Copy constructor.
		Handle(const Handle<T> &other) noexcept
		: unsafePointer(other.getUnsafe()) {
			if (this->unsafePointer != nullptr) {
				heap_increaseUseCount(this->unsafePointer);
			}
			#ifdef SAFE_POINTER_CHECKS
				this->name = other.getName();
				this->allocationIdentity = other.getAllocationIdentity();
				this->validate();
			#endif
		}
		// Copy constructor with static cast.
		template <typename V>
		Handle(const Handle<V> &other) noexcept
		: unsafePointer(static_cast<T*>(other.getUnsafe())) {
			if (this->unsafePointer != nullptr) {
				heap_increaseUseCount(this->unsafePointer);
			}
			#ifdef SAFE_POINTER_CHECKS
				this->name = other.getName();
				this->allocationIdentity = other.getAllocationIdentity();
				this->validate();
			#endif
		}
		// Move constructor.
		Handle(Handle<T> &&other) noexcept
		: unsafePointer(other.takeOwnership()) {
			#ifdef SAFE_POINTER_CHECKS
				this->name = other.getName();
				this->allocationIdentity = other.getAllocationIdentity();
				this->validate();
			#endif
		}
		// Move constructor with static cast.
		template <typename V>
		Handle(Handle<V> &&other) noexcept
		: unsafePointer(static_cast<T*>(other.takeOwnership())) {
			#ifdef SAFE_POINTER_CHECKS
				this->name = other.getName();
				this->allocationIdentity = other.getAllocationIdentity();
				this->validate();
			#endif
		}
		// Assignment.
		Handle<T>& operator = (const Handle<T> &other) {
			#ifdef SAFE_POINTER_CHECKS
				this->validate();
				this->name = other.getName();
				this->allocationIdentity = other.getAllocationIdentity();
			#endif
			if (this->unsafePointer != other.getUnsafe()) {
				// Decrease any old use count.
				if (this->unsafePointer != nullptr) {
					heap_decreaseUseCount(this->unsafePointer);
				}
				this->unsafePointer = other.unsafePointer;
				// Increase any new use count.
				if (this->unsafePointer != nullptr) {
					heap_increaseUseCount(this->unsafePointer);
				}
			}
			return *this;
		}
		// Assignment with static cast.
		template <typename V>
		Handle<T>& operator = (const Handle<V> &other) {
			#ifdef SAFE_POINTER_CHECKS
				this->validate();
				this->name = other.getName();
				this->allocationIdentity = other.getAllocationIdentity();
			#endif
			if (this->unsafePointer != other.getUnsafe()) {
				// Decrease any old use count.
				if (this->unsafePointer != nullptr) {
					heap_decreaseUseCount(this->unsafePointer);
				}
				this->unsafePointer = static_cast<T*>(other.unsafePointer);
				// Increase any new use count.
				if (this->unsafePointer != nullptr) {
					heap_increaseUseCount(this->unsafePointer);
				}
			}
			return *this;
		}
		// Move assignment.
		Handle<T>& operator = (Handle<T> &&other) {
			T* inherited = other.takeOwnership();
			#ifdef SAFE_POINTER_CHECKS
				this->validate();
				this->name = other.getName();
				this->allocationIdentity = other.getAllocationIdentity();
			#endif
			if (this->unsafePointer != inherited) {
				// Decrease any old use count.
				if (this->unsafePointer != nullptr) {
					heap_decreaseUseCount(this->unsafePointer);
				}
				this->unsafePointer = inherited;
			}
			return *this;
		}
		// Move assignment with static cast.
		template <typename V>
		Handle<T>& operator = (Handle<V> &&other) {
			T* inherited = static_cast<T*>(other.takeOwnership());
			#ifdef SAFE_POINTER_CHECKS
				this->validate();
				this->name = other.getName();
				this->allocationIdentity = other.getAllocationIdentity();
			#endif
			if (this->unsafePointer != inherited) {
				// Decrease any old use count.
				if (this->unsafePointer != nullptr) {
					heap_decreaseUseCount(this->unsafePointer);
				}
				this->unsafePointer = inherited;
			}
			return *this;
		}
		// Destructor.
		~Handle() {
			if (this->unsafePointer != nullptr) {
				#ifdef SAFE_POINTER_CHECKS
					this->validate();
				#endif
				heap_decreaseUseCount(this->unsafePointer);
			}
		}
		// Take ownership of the returned pointer from this handle.
		inline T* takeOwnership() {
			T* result = this->unsafePointer;
			this->unsafePointer = nullptr;
			return result;
		}
		// Check if the handle is null, using explicit syntax to explain the code.
		inline bool isNull() const { return this->unsafePointer == nullptr; }
		// Check if the handle points to anything, using explicit syntax to explain the code.
		inline bool isNotNull() const { return this->unsafePointer != nullptr; }
		// Check if the handle points to anything, using the handle as a boolean expression.
		inline operator bool() const { return this->unsafePointer != nullptr; }
		// Access content through the handle using the -> operator.
		inline T* operator ->() const {
			#ifdef SAFE_POINTER_CHECKS
				if (this->unsafePointer == nullptr) { impl_throwNullException(); }
				this->validate();
			#endif
			return this->unsafePointer;
		}
		// Returns the allocation's used size in bytes.
		inline uintptr_t getSize() const {
			return heap_getSize(this->unsafePointer);
		}
		// Get the number of elements by dividing the total size with the element size.
		inline uintptr_t getElementCount() const {
			// When sizeof(T) is a power of two, this unsigned integer division will be optimized into a bit shift by the compiler.
			return heap_getSize(this->unsafePointer) / sizeof(T);
		}
		// Get pointer.
		inline T* getUnsafe() const { return this->unsafePointer; }
		// Get reference.
		inline T& getReference() const {
			#ifdef SAFE_POINTER_CHECKS
				if (this->unsafePointer == nullptr) { impl_throwNullException(); }
				this->validate();
			#endif
			return *(this->unsafePointer);
		}
		// Get the use count.
		inline uintptr_t getUseCount() const {
			#ifdef SAFE_POINTER_CHECKS
				this->validate();
			#endif
			return this->unsafePointer ? heap_getUseCount(this->unsafePointer) : 0;
		}
	};

	// Construct a new unsafePointer of type T using the heap allocator and begin reference counting.
	// To avoid ambiguity with default and copy constructors, this constructor must be static.
	template<typename T, typename... ARGS>
	static Handle<T> handle_create(ARGS&&...args) {
		const char* name = "Handle object";
		// Reset the memory to zero before construction, in case that something was forgotten.
		// TODO: Should debug mode set the memory to a deterministic pattern to simplify detection of uninitialized variables?
		UnsafeAllocation allocation = heap_allocate(sizeof(T), true, name);
		// Construction from pointer increases the allocation's use count to 1.
		#ifdef SAFE_POINTER_CHECKS
			Handle<T> result((T*)(allocation.data), name, allocation.header->allocationIdentity);
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
		return std::move(result);
	}

	// Construct an array of objects with a shared handle pointing to the first element.
	// Pre-condition:
	//   T's size must be rounded by its own alignment, or alignof(T) will be violated when accessed as an array.
	//   sizeof(T) % alignof(T) == 0
	template<typename T, typename... ARGS>
	static Handle<T> handle_createArray(AllocationInitialization initialization, uintptr_t elementCount, ARGS&&...args) {
		const char* name = "Handle array";
		UnsafeAllocation allocation = heap_allocate(sizeof(T) * elementCount, initialization == AllocationInitialization::Zeroed, name);
		// Construction from pointer increases the allocation's use count to 1.
		#ifdef SAFE_POINTER_CHECKS
			Handle<T> result((T*)(allocation.data), name, allocation.header->allocationIdentity);
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
				// TODO: Can the element count be provided by the caller instead?
				//       It would make more sense when the destructor is called by the allocator that will later know the element count.
				heap_setAllocationDestructor(result.getUnsafe(), HeapDestructor([](void *toDestroy, void *externalResource) {
					// Calculate the number of elements from the size.
					intptr_t elementCount = heap_getSize(toDestroy) / sizeof(T);
					// Destroy each element.
					for (uintptr_t i = 0; i < elementCount; i++) {
						((T*)toDestroy)[i].~T();
					}
				}));
			}
		}
		return std::move(result);
	}

	// Dynamic casting of handles.
	//   OLD_TYPE does not have to be stated explicitly in the call, because it is provided by oldHandle.
	//   Example:
	//   Handle<TypeB> = handle_dynamicCast<TypeB>(TypeA(1, 2, 3));
	//   Pre-condition:
	//     The old handle must refer to a single element, or a null handle will be returned.
	//     oldhandle.getElementCount() == 1
	//   Post-condition:
	//     Returns oldHandle dynamically casted to NEW_TYPE.
	//     Returns an empty handle if the conversion failed.
	template <typename NEW_TYPE, typename OLD_TYPE>
	Handle<NEW_TYPE> handle_dynamicCast(const Handle<OLD_TYPE> &oldHandle) {
		#ifdef SAFE_POINTER_CHECKS
			if (oldHandle.getElementCount() != 1) {
				// Failed because the handle did not point to a single element.
				return Handle<NEW_TYPE>();
			} else {
				return Handle<NEW_TYPE>(dynamic_cast<NEW_TYPE*>(oldHandle.getUnsafe()), oldHandle.getName(), oldHandle.getAllocationIdentity());
			}
		#else
			return Handle<NEW_TYPE>(dynamic_cast<NEW_TYPE*>(oldHandle.getUnsafe()));
		#endif
	}
}

#endif
