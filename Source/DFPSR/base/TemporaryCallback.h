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

#ifndef DFPSR_TEMPORARY_CALLBACK
#define DFPSR_TEMPORARY_CALLBACK

#include <utility>

namespace dsr {

// TemporaryCallback is a reference to a callable function stored on the stack.
//   Because this type of callback is not allowed to be copied, only passed on by reference through calls, is is safe to capture variables by reference from a temporary context.
//   Either pass a lambda allocated on the stack or declare a new lambda for the implicit constructor.
//   Use as an input argument to high level functions when the function will not be saved for later.
//   Do not save a TemporaryCallback for later, because then it will refer to stack memory that is no longer available.
//     If TemporaryCallback outlives a lambda's closure allocated as a nameless structure on the stack, the stack memory will be deallocated before use.
//   If you store the callback then it is safer to use StorableCallback, because deleting the copy constructor can not prevent all memory errors.
template<typename T>
class TemporaryCallback;
template<typename RESULT, typename... ARGS>
class TemporaryCallback<RESULT(ARGS...)> {
private:
	// Declare the direct function pointer's type without any closure.
	using DirectFunction = RESULT(*)(ARGS... args);
	// Define a type for the invoking function where the closure is given explicitly.
	using InvokeFunction = RESULT(*)(const void *closure, ARGS... args);
	// A pointer to any closure data.
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
	// Pre-condition: functionPointer may not be null, because TemporaryCallback does not check for null when calling.
	TemporaryCallback(const DirectFunction functionPointer) {
		/* TODO: Can't call throwError from this header, because the string API depends on TemporaryCallback for string splitting.
		#ifndef NDEBUG
			if (functionPointer == nullptr) {
				throwError(U"Tried to create a non-nullable TemporaryCallback from a null function pointer!\n");
			}
		#endif
		*/
		this->closure = nullptr;
		this->functionPointer = functionPointer;
	}
	// Constructor from anything callable that defines the () operator.
	template<typename F>
	TemporaryCallback(const F& f) {
		// Point to the closure directly where it is stored.
		this->closure = &f;
		// Construct an invoking function that casts the unknown data into a know closure type to call.
		this->invoke = [](const void *closure, ARGS... args) -> RESULT {
			// Convert closure pointer into callable lambda to allow invoking the callable () operand.
			return (*(F*)closure)(std::forward<ARGS>(args)...);
		};
	}
	
	// No default constructor.
	TemporaryCallback() = delete;
	// No copy construction, only pass it by reference from input arguments.
	TemporaryCallback(const TemporaryCallback &other) = delete;
	// No move construction, only pass it by reference from input arguments.
	TemporaryCallback(const TemporaryCallback &&other) = delete;
	// No copy assignment, only pass it by reference from input arguments.
	TemporaryCallback& operator = (TemporaryCallback &other) = delete;
	// No move assignment, only pass it by reference from input arguments.
	TemporaryCallback& operator = (TemporaryCallback &&other) = delete;
	inline bool hasClosure() const {
		return closure != nullptr;
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
};

}

#endif
