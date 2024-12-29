// zlib open source license
//
// Copyright (c) 2017 to 2024 David Forsgren Piuva
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

#ifdef SAFE_POINTER_CHECKS
	#include <thread>
	#include <mutex>
#endif

using namespace dsr;

// Thread hash of memory without any specific owner.
static uint64_t ANY_THREAD_HASH = 0xF986BA1496E872A5;

#ifdef SAFE_POINTER_CHECKS
	// A primitive hash function that assumes that all compared objects have the same length, so that trailing zeroes can be ignored.
	static uint64_t hash(const uint8_t *bytes, size_t size) {
		uint64_t result = 527950984572370412;
		uint64_t a = 701348790128743674;
		uint64_t b = 418235620918472195;
		uint64_t c = 405871623857064987;
		uint64_t d = 685601283756306982;
		uint64_t e = 560123876058723749;
		uint64_t f = 123875604857293847;
		uint64_t g = 906123857648761038;
		uint64_t h = 720862395187683741;
		for (size_t byteIndex = 0; byteIndex < size; byteIndex++) {
			uint8_t byte = bytes[byteIndex];
			a = (a * 5819 + byteIndex * 75364 + 1746983) ^ 8761236358;
			b = (b * 4870 + byteIndex * 64294 + 6891364) ^ 2346987034;
			c = (c * 7059 + byteIndex * 91724 + 9234068) ^ 8016458371;
			d = (d * 2987 + byteIndex * 35729 + 5298712) ^ 1589721358;
			e = (e * 6198 + byteIndex * 11635 + 6349823) ^ 2938479216;
			f = (f * 5613 + byteIndex * 31873 + 7468895) ^ 5368713452;
			g = (g * 7462 + byteIndex * 98271 + 1287650) ^ 9120572938;
			h = (h * 1670 + byteIndex * 37488 + 6361083) ^ 4867350662;
			if (byte &   1) result = result ^ a;
			if (byte &   2) result = result ^ b;
			if (byte &   4) result = result ^ c;
			if (byte &   8) result = result ^ d;
			if (byte &  16) result = result ^ e;
			if (byte &  32) result = result ^ f;
			if (byte &  64) result = result ^ g;
			if (byte & 128) result = result ^ h;
		}
		return result;
	}

	// Hashed thread identity.
	static uint64_t createThreadHash() {
		std::thread::id id = std::this_thread::get_id();
		const uint8_t *bytes = (const uint8_t*)&id;
		return hash(bytes, sizeof(std::thread::id));
	}
	thread_local const uint64_t currentThreadHash = createThreadHash();

	// Globally unique identifiers for memory allocations.
	// Different allocations can have the same address at different times when allocations are recycled,
	//   so a globally unique identifier is needed to make sure that we access the same allocation.
	// We start at a constant of high entropy to minimize the risk of accidental matches and then increase by one in modulo 2⁶⁴ to prevent repetition of the exact same value.
	static std::mutex idLock;
	static uint64_t idCounter = 0xD13A98271E08BF57;
	static uint64_t createIdentity() {
		uint64_t result;
		idLock.lock();
			result = idCounter;
			idCounter++;
		idLock.unlock();
		return result;
	}

	AllocationHeader::AllocationHeader()
	: totalSize(0), threadHash(0), allocationIdentity(0) {}

	AllocationHeader::AllocationHeader(uintptr_t totalSize, bool threadLocal)
	: totalSize(totalSize), threadHash(threadLocal ? currentThreadHash : ANY_THREAD_HASH), allocationIdentity(createIdentity()) {}
#else
	AllocationHeader::AllocationHeader()
	: totalSize(0) {}

	AllocationHeader::AllocationHeader(uintptr_t totalSize, bool threadLocal)
	: totalSize(totalSize) {}
#endif

#ifdef SAFE_POINTER_CHECKS
	void dsr::assertNonNegativeSize(intptr_t size) {
		if (size < 0) {
			throwError(U"Negative size of SafePointer!\n");
		}
	}

	void dsr::assertInsideSafePointer(const char* method, const char* name, const uint8_t* pointer, const uint8_t* data, const uint8_t* regionStart, const uint8_t* regionEnd, const AllocationHeader *header, uint64_t allocationIdentity, intptr_t claimedSize, intptr_t elementSize) {
		if (regionStart == nullptr) {
			throwError(U"SafePointer exception! Tried to use a null pointer!\n");
			return;
		}
		// If the pointer has an allocation header, check that the identity matches the one stored in the pointer.
		if (header != nullptr) {
			uint64_t headerIdentity, headerHash;
			try {
				// Both allocation identity and thread hash may match by mistake, but in most of the cases this will give more information about why it happened.
				headerIdentity = header->allocationIdentity;
				headerHash = header->threadHash;
			} catch(...) {
				throwError(U"SafePointer exception! Tried to access memory not available to the application!\n");
				return;
			}
			if (headerIdentity != allocationIdentity) {
				throwError(U"SafePointer exception! Accessing freed memory or corrupted allocation header!\n  headerIdentity = ", headerIdentity, U"\n  allocationIdentity = ", allocationIdentity, U"");
				return;
			} else if (headerHash != ANY_THREAD_HASH && headerHash != currentThreadHash) {
				throwError(U"SafePointer exception! Accessing another thread's private memory!\n  headerHash = ", headerHash, U"\n  currentThreadHash = ", currentThreadHash, U"\n");
				return;
			}
		}
		if (pointer < regionStart || pointer + claimedSize > regionEnd) {
			String message;
			string_append(message, U"\n _________________ SafePointer out of bound exception! _________________\n");
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
			return;
		}
	}
#endif
