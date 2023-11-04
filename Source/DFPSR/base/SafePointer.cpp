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

#include "SafePointer.h"
#include "../api/stringAPI.h"

using namespace dsr;

void dsr::assertNonNegativeSize(intptr_t size) {
	if (size < 0) {
		throwError(U"Negative size of SafePointer!\n");
	}
}

void dsr::assertInsideSafePointer(const char* method, const char* name, const uint8_t* pointer, const uint8_t* data, const uint8_t* regionStart, const uint8_t* regionEnd, intptr_t claimedSize, intptr_t elementSize) {
	if (pointer < regionStart || pointer + claimedSize > regionEnd) {
		String message;
		if (data == nullptr) {
			string_append(message, U"\n _____________________ SafePointer null exception! _____________________\n");
		} else {
			string_append(message, U"\n _________________ SafePointer out of bound exception! _________________\n");
		}
		string_append(message, U"/\n");
		string_append(message, U"|  Name: ", name, U"\n");
		string_append(message, U"|  Method: ", method, U"\n");
		string_append(message, U"|  Region: ", (uintptr_t)regionStart, U" to ", (uintptr_t)regionEnd, U"\n");
		string_append(message, U"|  Region size: ", (intptr_t)(regionEnd - regionStart), U" bytes\n");
		string_append(message, U"|  Base pointer: ", (uintptr_t)data, U"\n");
		string_append(message, U"|  Requested pointer: ", (uintptr_t)pointer, U"\n");
		string_append(message, U"|  Requested size: ", claimedSize, U" bytes\n");

		intptr_t startOffset = (intptr_t)pointer - (intptr_t)regionStart;
		intptr_t baseOffset = (intptr_t)pointer - (intptr_t)data;

		// Index relative to allocation start
		//   regionStart is the start of the accessible memory region
		if (startOffset != baseOffset) {
			string_append(message, U"|  Start offset: ", startOffset, U" bytes\n");
			if (startOffset % elementSize == 0) {
				intptr_t index = startOffset / elementSize;
				intptr_t elementCount = ((intptr_t)regionEnd - (intptr_t)regionStart) / elementSize;
				string_append(message, U"|    Start index: ", index, U" [0..", (elementCount - 1), U"]\n");
			}
		}

		// Base index relative to the stored pointer within the region
		//   data is the base of the allocation at index zero
		string_append(message, U"|  Base offset: ", baseOffset, U" bytes\n");
		if (baseOffset % elementSize == 0) {
			intptr_t index = baseOffset / elementSize;
			intptr_t elementCount = ((intptr_t)regionEnd - (intptr_t)data) / elementSize;
			string_append(message, U"|    Base index: ", index, U" [0..", (elementCount - 1), U"]\n");
		}
		string_append(message, U"\\_______________________________________________________________________\n\n");
		throwError(message);
	}
}

