// zlib open source license
//
// Copyright (c) 2019 David Forsgren Piuva
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

#ifndef DFPSR_API_MEDIA_MACHINE
#define DFPSR_API_MEDIA_MACHINE

#include "../implementation/image/Image.h"
#include "../base/Handle.h"
#include "../math/FixedPoint.h"

namespace dsr {

// A handle to a media machine.
//   Media machines can be used to generate, filter and analyze images.
//   Everything running in a media machine is guaranteed to be 100% deterministic to the last bit.
//     This reduces the amount of code where maintenance has to be performed during porting.
//     It also means that any use of float or double is forbidden.
struct VirtualMachine;
using MediaMachine = Handle<VirtualMachine>;

// TODO: Complete VirtualMachine with conditional jumps and document the language dialect used by MediaMachine.
// Side-effect: Creates a media machine from Media Machine Code (*.mmc file).
// Post-condition: Returns a reference counted MediaMachine handle to the virtual machine. 
MediaMachine machine_create(const ReadableString& code);

// Post-condition: Returns true iff machine exists.
bool machine_exists(const MediaMachine& machine);

// Pre-condition:
//   * The machine must exist.
//     machine_exists(machine)
// Returns the index to the method who's name matches methodName with case insensitivity in machine, or -1 if it does not exist in machine.
int32_t machine_findMethod(const MediaMachine& machine, const ReadableString& methodName);

// Assign an input argument of a method before your call to machine_executeMethod.
// Pre-condition:
//   * The machine must exist.
//     machine_exists(machine)
//   * methodIndex must refer to the method to call in machine.
//     0 <= methodIndex < machine_getMethodCount(machine)
//   * inputIndex must refer to an input of the method in machine.
//     0 <= inputIndex < machine_getInputCount(machine, methodIndex)
// Side-effect: Sets the input at inputIndex in machine's method at methodIndex to input.
void machine_setInputByIndex(MediaMachine& machine, int32_t methodIndex, int32_t inputIndex, int32_t input);
void machine_setInputByIndex(MediaMachine& machine, int32_t methodIndex, int32_t inputIndex, const FixedPoint& input);
void machine_setInputByIndex(MediaMachine& machine, int32_t methodIndex, int32_t inputIndex, const AlignedImageU8& input);
void machine_setInputByIndex(MediaMachine& machine, int32_t methodIndex, int32_t inputIndex, const OrderedImageRgbaU8& input);

// Call a method in the media machine, reading from input registers and writing to output registers.
// Pre-condition:
//   * The machine must exist.
//     machine_exists(machine)
//   * All inputs of the method must be assigned before the call using the same methodIndex.
//   * methodIndex must refer to the method to call in machine.
//     0 <= methodIndex < machine_getMethodCount(machine)
// Side-effect: Writes the results to output arguments.
void machine_executeMethod(MediaMachine& machine, int32_t methodIndex);

// Read output register at outputIndex
// Pre-condition:
//   * The machine must exist.
//     machine_exists(machine)
//   * methodIndex must refer to the method in machine's last call to machine_executeMethod.
//     0 <= methodIndex < machine_getMethodCount(machine)
//   * The output index must be within range.
//     0 <= outputIndex < machine_getOutputCount(machine, methodIndex)
// Post-condition: Returns the output at outputIndex.
FixedPoint machine_getFixedPointOutputByIndex(const MediaMachine& machine, int32_t methodIndex, int32_t outputIndex);
AlignedImageU8 machine_getImageU8OutputByIndex(const MediaMachine& machine, int32_t methodIndex, int32_t outputIndex);
OrderedImageRgbaU8 machine_getImageRgbaU8OutputByIndex(const MediaMachine& machine, int32_t methodIndex, int32_t outputIndex);

// Pre-condition:
//   * The machine must exist.
//     machine_exists(machine)
// Post-condition: Returns the number of methods in machine.
int32_t machine_getMethodCount(const MediaMachine& machine);

// Pre-condition:
//   * The machine must exist.
//     machine_exists(machine)
//   * methodIndex must refer to the method that you want to get the name from.
//     0 <= methodIndex < machine_getMethodCount(machine)
// Post-condition: Returns the name of the method at methodIndex in machine.
String machine_getMethodName(const MediaMachine& machine, int32_t methodIndex);

// Pre-condition:
//   * The machine must exist.
//     machine_exists(machine)
//   * methodIndex must refer to the method that you want to get the input count from.
//     0 <= methodIndex < machine_getMethodCount(machine)
// Post-condition: Returns the input argument count for the method at methodIndex in machine.
int32_t machine_getInputCount(const MediaMachine& machine, int32_t methodIndex);

// Pre-condition:
//   * The machine must exist.
//     machine_exists(machine)
//   * methodIndex must refer to the method that you want to get the output count from.
//     0 <= methodIndex < machine_getMethodCount(machine)
// Post-condition: Returns the output argument count for the method at methodIndex in machine.
int32_t machine_getOutputCount(const MediaMachine& machine, int32_t methodIndex);

// Pre-condition:
//   * The machine must exist.
//     machine_exists(machine)
//   * methodIndex must refer to the method that you want to get an input argument's name from.
//     0 <= methodIndex < machine_getMethodCount(machine)
//   * The inputIndex must refer to the input argument that you want to get the name of.
//     0 <= inputIndex < machine_getInputCount(machine, methodIndex)
// Post-condition: Returns the input argument name at inputIndex for the method at methodIndex in machine.
String machine_getInputName(const MediaMachine& machine, int32_t methodIndex, int32_t inputIndex);

// Pre-condition:
//   * The machine must exist.
//     machine_exists(machine)
//   * methodIndex must refer to the method that you want to get an output argument's name from.
//     0 <= methodIndex < machine_getMethodCount(machine)
//   * The outputIndex must refer to the output argument that you want to get the name of.
//     0 <= outputIndex < machine_getOutputCount(machine, methodIndex)
// Post-condition: Returns the output argument name at outputIndex for the method at methodIndex in machine.
String machine_getOutputName(const MediaMachine& machine, int32_t methodIndex, int32_t outputIndex);

// Helper function counting the number of arguments given to it.
inline constexpr int32_t machine_argCount() {
	return 0;
}
template<typename HEAD, typename... TAIL>
inline constexpr int32_t machine_argCount(HEAD& first, TAIL&... args) {
	return machine_argCount(args...) + 1;
}

// A temporary type generated from () calls to MediaMethod, which is used for writing outputs to targets within the next ().
class MediaResult {
private:
	// Holding the machine by reference prevents storing MediaResult,
	//   because it may not be used after other calls to the machine.
	// It is only used to assign outputs with the () operator.
	MediaMachine &machine;
	int32_t methodIndex;
	void writeResult(int32_t outputIndex, int8_t& target) {
		target = fixedPoint_round(machine_getFixedPointOutputByIndex(this->machine, this->methodIndex, outputIndex));
	}
	void writeResult(int32_t outputIndex, int16_t& target) {
		target = fixedPoint_round(machine_getFixedPointOutputByIndex(this->machine, this->methodIndex, outputIndex));
	}
	void writeResult(int32_t outputIndex, int32_t& target) {
		target = fixedPoint_round(machine_getFixedPointOutputByIndex(this->machine, this->methodIndex, outputIndex));
	}
	void writeResult(int32_t outputIndex, int64_t& target) {
		target = fixedPoint_round(machine_getFixedPointOutputByIndex(this->machine, this->methodIndex, outputIndex));
	}
	void writeResult(int32_t outputIndex, FixedPoint& target) {
		target = machine_getFixedPointOutputByIndex(this->machine, this->methodIndex, outputIndex);
	}
	void writeResult(int32_t outputIndex, AlignedImageU8& target) {
		target = machine_getImageU8OutputByIndex(this->machine, this->methodIndex, outputIndex);
	}
	void writeResult(int32_t outputIndex, OrderedImageRgbaU8& target) {
		target = machine_getImageRgbaU8OutputByIndex(this->machine, this->methodIndex, outputIndex);
	}
	inline void writeResults(int32_t firstInputIndex) {}
	template<typename HEAD, typename... TAIL>
	inline void writeResults(int32_t firstInputIndex, HEAD& first, TAIL&... args) {
		this->writeResult(firstInputIndex, first);
		this->writeResults(firstInputIndex + 1, args...);
	}
public:
	MediaResult(MediaMachine& machine, int32_t methodIndex)
 	: machine(machine), methodIndex(methodIndex) {}
	// Write target references within () after a call to assign multiple outputs
	template <typename... ARGS>
	void operator () (ARGS&... args) {
		int32_t givenCount = machine_argCount(args...);
		int32_t expectedCount = machine_getOutputCount(this->machine, this->methodIndex);
		if (givenCount != expectedCount) {
			throwError(U"The call to ", machine_getMethodName(this->machine, this->methodIndex), U" expected ", expectedCount, U" outputs, but ", givenCount, U" references were assigned.\n");
		}
		this->writeResults(0, args...);
	}
};

// How to call a MediaMethod:
// * Using arguments in the same order as declared in the Media Machine Code.
//     myMediaMethod(inputA, inputB, inputC...)(outputX, outputY...)
//     This allow returning more than one return value without having to declare any structures in the virtual machine.
// * Using keyword arguments.
//     You can also call myMediaMethod and just define a lambda that is called with the argument name and index for each input argument.
//     Then your function calls machine_setInputByIndex with the given arguments and the input data identified by name.
class MediaMethod {
public:
	MediaMachine machine;
	int32_t methodIndex; // Index of the method being called.
	int32_t contextIndex; // Index used to know from which context implicit variables are being fetched.
private:
	inline void setInputs(int32_t firstInputIndex) {}
	template<typename HEAD, typename... TAIL>
	inline void setInputs(int32_t firstInputIndex, const HEAD &first, TAIL&&... args) {
		machine_setInputByIndex(this->machine, this->methodIndex, firstInputIndex, first);
		this->setInputs(firstInputIndex + 1, args...);
	}
public:
	MediaMethod()
 	: methodIndex(-1), contextIndex(0) {}
	MediaMethod(const MediaMachine& machine, int32_t methodIndex, int32_t contextIndex)
 	: machine(machine), methodIndex(methodIndex), contextIndex(contextIndex) {}
	// MediaMethod can be called like a function using arguments, returning MediaResult for assigning outputs by reference. 
	// Useful when you know the arguments in advance.
	template <typename... ARGS>
	MediaResult operator () (ARGS&&... args) {
		int32_t givenCount = machine_argCount(args...);
		int32_t expectedCount = machine_getInputCount(this->machine, this->methodIndex);
		if (givenCount != expectedCount) {
			throwError(U"The call to ", machine_getMethodName(this->machine, this->methodIndex), U" expected ", expectedCount, U" inputs, but ", givenCount, U" values were given.\n");
		}
		this->setInputs(0, args...);
		machine_executeMethod(this->machine, this->methodIndex);
		return MediaResult(this->machine, this->methodIndex);
	}
	// MediaMethod can also take the inputs as keyword arguments by getting a callback with the index and name of each input to assign.
	// The function setInputAction should simply make a call to machine_setInputByIndex with the provided machine, methodIndex, inputIndex and the value corresponding to argumentName in setInputAction.
	// If you don't recognize argumentName, then throw an exception because default input arguments are currently not implemented.
	// Useful when the called function can be extended or reduced with only the arguments needed.
	MediaResult callUsingKeywords(std::function<void(MediaMachine &machine, int32_t methodIndex, int32_t inputIndex, const ReadableString &argumentName)> setInputAction);
};

// Post-condition: Returns a MediaMethod structure, which can be stored as a reference counting function pointer that keeps the virtual machine alive.
MediaMethod machine_getMethod(MediaMachine& machine, const ReadableString& methodName, int32_t contextIndex, bool mustExist = true);

}

#endif
