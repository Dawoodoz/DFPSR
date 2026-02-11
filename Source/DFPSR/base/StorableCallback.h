// zlib open source license
//
// Copyright (c) 2026 David Forsgren Piuva
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

#ifndef DFPSR_STORABLE_CALLBACK
#define DFPSR_STORABLE_CALLBACK

#include "heap.h"
#include "../api/stringAPI.h"
#include <utility>

// If creating a StorableCallback with a lambda that does not capture anything, a heap allocation will still be made, because a null closure indicate that the content is a function pointer.
//   To avoid that, one can point to a function instead of using a lambda.

namespace dsr {

// The slow but powerful callback that copies the lambda's closure to a heap allocation.
//   Because it may be saved for later, it is recommended to capture variables by value or reference counted handles to prevent the creation of dangling references to freed stack memory.
//   Raw pointers and references to stack memory will go out of scope, defeating the purpose of cloning the closure into heap memory.
template<typename T>
class StorableCallback;
template<typename RESULT, typename... ARGS>
class StorableCallback<RESULT(ARGS...)> {
private:
	// Declare the direct function pointer's type without any closure.
	using DirectFunction = RESULT(*)(ARGS... args);
	// Define a type for the invoking function where the closure is given explicitly.
	using InvokeFunction = RESULT(*)(const void *closure, ARGS... args);
	// A pointer to any closure data that must be allocated through heap.h and be manually reference counted.
	// TODO: Give a function for destroying the lambda copy.
	const void *closure = nullptr;
	// When closure is null, this pointer is a function pointer to call directly.
	union {
		// A pointer to the invoke function, which calls a lambda or function pointer.
		DirectFunction functionPointer;
		// A pointer to the invoke function, which calls a lambda or function pointer.
		InvokeFunction invoke;
	};
public:
	// Constructor from function pointer without any closure.
	// Pre-condition: functionPointer may not be null, because StorableCallback does not check for null when calling.
	StorableCallback(const DirectFunction functionPointer) {
		#ifndef NDEBUG
			if (functionPointer == nullptr) {
				throwError(U"Tried to create a non-nullable StorableCallback from a null function pointer! Pass a function pointer that prints an error message if you don't want it to be called, or an empty function if you don't want anything to happen.\n");
			}
		#endif
		this->closure = nullptr;
		this->functionPointer = functionPointer;
	}
	template<typename F>
	StorableCallback(const F& f) {
		// Allocate heap memory for the closure with all bytes initialize to zero for determinism.
		UnsafeAllocation allocation = heap_allocate(sizeof(F), true);
		if (allocation.data == nullptr) {
			throwError(U"Failed to allocate ", sizeof(F), U" bytes of memory for a closure in StorableCallback!\n");
		} else {
			// Use the allocation as the callback's closure.
			this->closure = allocation.data;
			// Cast to the intended type.
			F *newClosure = (F*)(allocation.data);
			// Increase reference count to one.
			heap_increaseUseCount(this->closure);
			// Assign a debug name if running in debug mode.
			#ifdef SAFE_POINTER_CHECKS
				heap_setAllocationName(this->closure, "StorableCallback closure");
			#endif
			// Call the lambda's copy constructor in-place using our own memory allocation.
			new (newClosure) F(f);
			// If the type has a destructor, then we must provide it to the allocator.
			if (!std::is_trivially_destructible<F>::value) {
				heap_setAllocationDestructor(newClosure, HeapDestructor([](void *toDestroy, void *externalResource) {
					// Destroy one object.
					((F*)toDestroy)->~F();
				}));
			}
			// Construct an invoking function that casts the unknown data into a know closure type to call.
			this->invoke = [](const void *closure, ARGS... args) -> RESULT {
				// Convert closure pointer into callable lambda to allow invoking the callable () operand.
				return (*(F*)closure)(std::forward<ARGS>(args)...);
			};
		}
	}
	// Copy constructor.
	StorableCallback(const StorableCallback &other) noexcept
	: closure(other.closure), functionPointer(other.functionPointer) {
		if (this->closure != nullptr) {
			heap_increaseUseCount(this->closure);
		}
	}
	// Move constructor.
	StorableCallback(StorableCallback &&other) noexcept
	: closure(other.closure), functionPointer(other.functionPointer) {
		other.closure = nullptr;
	}
	// Copy assignment.
	StorableCallback& operator = (const StorableCallback &other) {
		this->functionPointer = other.functionPointer;
		if (this->closure != other.closure) {
			// Decrease any old use count.
			if (this->closure != nullptr) {
				heap_decreaseUseCount(this->closure);
			}
			this->closure = other.closure;
			// Increase any new use count.
			if (this->closure != nullptr) {
				heap_increaseUseCount(this->closure);
			}
		}
		return *this;
	}
	// Move assignment.
	StorableCallback& operator = (StorableCallback &&other) {
		this->functionPointer = other.functionPointer;
		const void *inherited = other.closure;
		other.closure = nullptr;
		if (this->closure != inherited) {
			// Decrease any old use count.
			if (this->closure != nullptr) {
				heap_decreaseUseCount(this->closure);
			}
			this->closure = inherited;
		}
		return *this;
	}
	inline bool hasClosure() const {
		return this->closure != nullptr;
	}
	inline uintptr_t getClosureUseCount() const {
		return (this->closure != nullptr) ? heap_getUseCount(this->closure) : 0;
	}
	inline RESULT operator()(ARGS... args) const {
		// Because a callback may not be empty, we only have to check if it has a lambda closure or not.
		if (this->hasClosure()) {
			// Call the invoke function that remembers the nameless lambda type and provides the data.
			return this->invoke(this->closure, std::forward<ARGS>(args)...);
		} else {
			// Call the function pointer directly.
			return (*this->functionPointer)(std::forward<ARGS>(args)...);
		}
	}
	~StorableCallback() {
		// If the callback has a closure, then decrease the closure's reference count, so that it will be freed once no callback is using it anymore.
		if (this->closure != nullptr) {
			heap_decreaseUseCount(this->closure);
		}
	};
};

}

#endif
