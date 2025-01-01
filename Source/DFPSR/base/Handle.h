// zlib open source license
//
// Copyright (c) 2024 David Forsgren Piuva
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

namespace dsr {
	template <typename T>
	struct Handle {
		// The internal pointer that reference counting is added to.
		//   Must be allocated using heap_allocate, so that it can be freed using heap_free when the use count reaches zero.
		T *unsafePointer = nullptr;
		// Default construct an empty handle.
		Handle() {}
		// Construct from pointer.
		//   Pre-condition: unsafePointer is the data allocated with heap_allocate.
		Handle(T* unsafePointer) noexcept
		: unsafePointer(unsafePointer) {
			if (this->unsafePointer != nullptr) {
				heap_increaseUseCount((uint8_t*)this->unsafePointer);
			}
		}
		// Copy constructor.
		Handle(const Handle<T> &other) noexcept
		: unsafePointer(other.unsafePointer) {
			if (this->unsafePointer != nullptr) {
				heap_increaseUseCount((uint8_t*)this->unsafePointer);
			}
		}
		// Copy constructor with static cast.
		template <typename V>
		Handle(const Handle<V> &other) noexcept
		: unsafePointer(static_cast<T*>(other.unsafePointer)) {
			if (this->unsafePointer != nullptr) {
				heap_increaseUseCount((uint8_t*)this->unsafePointer);
			}
		}
		// Move constructor.
		Handle(Handle<T> &&other) noexcept
		: unsafePointer(other.unsafePointer) {
			other.unsafePointer = nullptr;
		}
		// Move constructor with static cast.
		template <typename V>
		Handle(Handle<V> &&other) noexcept
		: unsafePointer(static_cast<T*>(other.unsafePointer)) {
			other.unsafePointer = nullptr;
		}
		// Assignment.
		Handle<T>& operator = (const Handle<T> &other) {
			if (this->unsafePointer != other.unsafePointer) {
				// Decrease any old use count.
				if (this->unsafePointer != nullptr) {
					heap_decreaseUseCount((uint8_t*)this->unsafePointer);
				}
				this->unsafePointer = other.unsafePointer;
				// Increase any new use count.
				if (this->unsafePointer != nullptr) {
					heap_increaseUseCount((uint8_t*)this->unsafePointer);
				}
			}
			return *this;
		}
		// Assignment with static cast.
		template <typename V>
		Handle<T>& operator = (const Handle<V> &other) {
			if (this->unsafePointer != other.unsafePointer) {
				// Decrease any old use count.
				if (this->unsafePointer != nullptr) {
					heap_decreaseUseCount((uint8_t*)this->unsafePointer);
				}
				this->unsafePointer = static_cast<T*>(other.unsafePointer);
				// Increase any new use count.
				if (this->unsafePointer != nullptr) {
					heap_increaseUseCount((uint8_t*)this->unsafePointer);
				}
			}
			return *this;
		}
		// Destructor.
		~Handle() {
			if (this->unsafePointer != nullptr) {
				heap_decreaseUseCount((uint8_t*)this->unsafePointer);
			}
		}
		// Check if the handle is null, using explicit syntax to explain the code.
		inline bool isNull() const { return this->unsafePointer == nullptr; }
		// Check if the handle points to anything, using explicit syntax to explain the code.
		inline bool isNotNull() const { return this->unsafePointer != nullptr; }
		// Check if the handle points to anything, using explicit syntax to explain the code.
		inline bool exists() const { return this->unsafePointer != nullptr; } // TODO: Replace with isNotNull for consistency with SafePointer.
		// Check if the handle points to anything, using the handle as a boolean expression.
		inline operator bool() const { return this->unsafePointer != nullptr; }
		// Access content through the handle using the -> operator.
		inline T* operator ->() const {
			#ifdef SAFE_POINTER_CHECKS
				if (this->unsafePointer == nullptr) { impl_throwNullException(); }
			#endif
			return this->unsafePointer;
		}
		// Get pointer.
		inline T* getUnsafe() const { return this->unsafePointer; }
		// Get reference.
		inline T& getReference() const {
			#ifdef SAFE_POINTER_CHECKS
				if (this->unsafePointer == nullptr) { impl_throwNullException(); }
			#endif
			return *(this->unsafePointer);
		}
		// Get the use count.
		inline uintptr_t getUseCount() const { return this->unsafePointer ? heap_getUseCount((uint8_t*)this->unsafePointer) : 0; }
	};

	// Construct a new unsafePointer of type T using the heap allocator and begin reference counting.
	// To avoid ambiguity with default and copy constructors, this constructor must be static.
	template<typename T, typename... ARGS>
	static Handle<T> handle_create(ARGS&&...args) {
		// Construction from pointer increases the allocation's use count to 1.
		Handle<T> result((T*)(heap_allocate(sizeof(T)).data));
		if (result.isNull()) {
			impl_throwAllocationFailure();
		} else {
			new (result.getUnsafe()) T(std::forward<ARGS>(args)...);
			heap_setAllocationDestructor((uint8_t*)result.getUnsafe(), [](void *toDestroy) {
				((T*)toDestroy)->~T();
			});
		}
		return std::move(result);
	}
	
	// Dynamic casting of handles.
	//   Returns an empty handle if the conversion failed.
	template <typename NEW_TYPE, typename OLD_TYPE>
	Handle<NEW_TYPE> handle_dynamicCast(const Handle<OLD_TYPE> &oldHandle) {
		return Handle<NEW_TYPE>(dynamic_cast<NEW_TYPE*>(oldHandle.getUnsafe()));
	}
}

#endif
