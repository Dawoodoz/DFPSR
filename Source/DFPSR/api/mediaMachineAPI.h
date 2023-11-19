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

#include "../math/FixedPoint.h"
#include "../api/types.h"

namespace dsr {

MediaMachine machine_create(const ReadableString& code);

int machine_findMethod(MediaMachine& machine, const ReadableString& methodName);
void machine_setInputByIndex(MediaMachine& machine, int methodIndex, int inputIndex, int32_t input);
void machine_setInputByIndex(MediaMachine& machine, int methodIndex, int inputIndex, const FixedPoint& input);
void machine_setInputByIndex(MediaMachine& machine, int methodIndex, int inputIndex, const AlignedImageU8& input);
void machine_setInputByIndex(MediaMachine& machine, int methodIndex, int inputIndex, const OrderedImageRgbaU8& input);
void machine_executeMethod(MediaMachine& machine, int methodIndex);
FixedPoint machine_getFixedPointOutputByIndex(MediaMachine& machine, int methodIndex, int outputIndex);
AlignedImageU8 machine_getImageU8OutputByIndex(MediaMachine& machine, int methodIndex, int outputIndex);
OrderedImageRgbaU8 machine_getImageRgbaU8OutputByIndex(MediaMachine& machine, int methodIndex, int outputIndex);
String machine_getMethodName(MediaMachine& machine, int methodIndex);
int machine_getInputCount(MediaMachine& machine, int methodIndex);
int machine_getOutputCount(MediaMachine& machine, int methodIndex);
String machine_getInputName(MediaMachine& machine, int methodIndex, int inputIndex);
String machine_getOutputName(MediaMachine& machine, int methodIndex, int outputIndex);

inline constexpr int argCount() {
	return 0;
}
template<typename HEAD, typename... TAIL>
inline constexpr int argCount(HEAD& first, TAIL&... args) {
	return argCount(args...) + 1;
}

// TODO: Prevent saving the result to avoid reading after another call
class MediaResult {
private:
	MediaMachine machine;
	int methodIndex;
	void writeResult(int outputIndex, int8_t& target) {
		target = fixedPoint_round(machine_getFixedPointOutputByIndex(this->machine, this->methodIndex, outputIndex));
	}
	void writeResult(int outputIndex, int16_t& target) {
		target = fixedPoint_round(machine_getFixedPointOutputByIndex(this->machine, this->methodIndex, outputIndex));
	}
	void writeResult(int outputIndex, int32_t& target) {
		target = fixedPoint_round(machine_getFixedPointOutputByIndex(this->machine, this->methodIndex, outputIndex));
	}
	void writeResult(int outputIndex, int64_t& target) {
		target = fixedPoint_round(machine_getFixedPointOutputByIndex(this->machine, this->methodIndex, outputIndex));
	}
	void writeResult(int outputIndex, FixedPoint& target) {
		target = machine_getFixedPointOutputByIndex(this->machine, this->methodIndex, outputIndex);
	}
	void writeResult(int outputIndex, AlignedImageU8& target) {
		target = machine_getImageU8OutputByIndex(this->machine, this->methodIndex, outputIndex);
	}
	void writeResult(int outputIndex, OrderedImageRgbaU8& target) {
		target = machine_getImageRgbaU8OutputByIndex(this->machine, this->methodIndex, outputIndex);
	}
	inline void writeResults(int firstInputIndex) {}
	template<typename HEAD, typename... TAIL>
	inline void writeResults(int firstInputIndex, HEAD& first, TAIL&... args) {
		this->writeResult(firstInputIndex, first);
		this->writeResults(firstInputIndex + 1, args...);
	}
public:
	MediaResult(const MediaMachine& machine, int methodIndex)
 	: machine(machine), methodIndex(methodIndex) {}
	// Write target references within () after a call to assign multiple outputs
	template <typename... ARGS>
	void operator () (ARGS&... args) {
		int givenCount = argCount(args...);
		int expectedCount = machine_getOutputCount(this->machine, this->methodIndex);
		if (givenCount != expectedCount) {
			throwError("The call to ", machine_getMethodName(this->machine, this->methodIndex), " expected ", expectedCount, " outputs, but ", givenCount, " references were assigned.\n");
		}
		this->writeResults(0, args...);
	}
};

class MediaMethod {
public:
	MediaMachine machine;
	int methodIndex; // Index of the method being called.
	int contextIndex; // Index used to know from which context implicit variables are being fetched.
private:
	inline void setInputs(int firstInputIndex) {}
	template<typename HEAD, typename... TAIL>
	inline void setInputs(int firstInputIndex, const HEAD &first, TAIL&&... args) {
		machine_setInputByIndex(this->machine, this->methodIndex, firstInputIndex, first);
		this->setInputs(firstInputIndex + 1, args...);
	}
public:
	MediaMethod()
 	: methodIndex(-1), contextIndex(0) {}
	MediaMethod(const MediaMachine& machine, int methodIndex, int contextIndex)
 	: machine(machine), methodIndex(methodIndex), contextIndex(contextIndex) {}
	// MediaMethod can be called like a function using arguments, returning MediaResult for assigning outputs by reference. 
	// Useful when you know the arguments in advance.
	template <typename... ARGS>
	MediaResult operator () (ARGS&&... args) {
		int givenCount = argCount(args...);
		int expectedCount = machine_getInputCount(this->machine, this->methodIndex);
		if (givenCount != expectedCount) {
			throwError("The call to ", machine_getMethodName(this->machine, this->methodIndex), " expected ", expectedCount, " inputs, but ", givenCount, " values were given.\n");
		}
		this->setInputs(0, args...);
		machine_executeMethod(this->machine, this->methodIndex);
		return MediaResult(this->machine, this->methodIndex);
	}
	// MediaMethod can also take the inputs as keyword arguments by getting a callback with the index and name of each input to assign.
	// The function setInputAction should simply make a call to machine_setInputByIndex with the provided machine, methodIndex, inputIndex and the value corresponding to argumentName in setInputAction.
	// If you don't recognize argumentName, then throw an exception because default input arguments are currently not implemented.
	// Useful when the called function can be extended or reduced with only the arguments needed.
	MediaResult callUsingKeywords(std::function<void(MediaMachine &machine, int methodIndex, int inputIndex, const ReadableString &argumentName)> setInputAction);
};

MediaMethod machine_getMethod(MediaMachine& machine, const ReadableString& methodName, int contextIndex, bool mustExist = true);

}

#endif
