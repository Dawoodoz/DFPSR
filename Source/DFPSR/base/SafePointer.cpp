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

	AllocationHeader::AllocationHeader(uintptr_t totalSize, bool threadLocal, const char *name)
	: totalSize(totalSize), name(name), threadHash(threadLocal ? currentThreadHash : ANY_THREAD_HASH), allocationIdentity(createIdentity()) {}
#else
	AllocationHeader::AllocationHeader()
	: totalSize(0) {}

	// TODO: Avoid passing the debug name in release mode by placing these functions in memory.h.
	//       Create separate methods for getting the thread hash and the next allocation nonce.
	AllocationHeader::AllocationHeader(uintptr_t totalSize, bool threadLocal, const char *name)
	: totalSize(totalSize) {}
#endif

#ifdef SAFE_POINTER_CHECKS
	void dsr::impl_assertNonNegativeSize(intptr_t size) {
		if (size < 0) {
			throwError(U"Negative size of SafePointer!\n");
		}
	}

	static bool isOutOfBound(const uint8_t* claimedStart, intptr_t claimedSize, const uint8_t* regionStart, const uint8_t* regionEnd) {
		return claimedStart < regionStart || claimedStart + claimedSize > regionEnd;
	}

	static void throwPointerError(const ReadableString &title, const char* method, const char* name, const uint8_t* pointer, const uint8_t* regionStart, const uint8_t* regionEnd, const AllocationHeader *header, uint64_t allocationIdentity, intptr_t claimedSize, intptr_t elementSize, uint64_t headerIdentity, uint64_t headerHash) {
		bool outOfBound = isOutOfBound(pointer, claimedSize, regionStart, regionEnd);
		String message;
		string_append(message, U"\n _______________________________________________________________________\n");
		string_append(message, U"/\n");
		string_append(message, U"|  ", title, U"\n");
		string_append(message, U"|\n");
		string_append(message, U"|  SafePointer operation: ", method, U"\n");
		string_append(message, U"|  Pointer name: ", name, U"\n");
		if (header != nullptr) {
			const char *allocationName = nullptr;
			try {
				allocationName = header->name;
			} catch(...) {
				allocationName = "<Invalid>";
			}
			string_append(message, U"|  Allocation name: ", allocationName, U"\n");
			string_append(message, U"|  Thread hash:\n");
			if (header->threadHash == ANY_THREAD_HASH) {
				string_append(message, U"|    Shared with all threads\n");
			} else {
				string_append(message, U"|    Owner thread: ", headerHash, U"\n");
				string_append(message, U"|    Calling thread: ", currentThreadHash, U"\n");
			}
			string_append(message, U"|  Identity:\n");
			string_append(message, U"|    Found: ", headerIdentity, U"\n");
			string_append(message, U"|    Expected: ", allocationIdentity, U"\n");
			// TODO: Check if the requested data is outside of the memory allocation's used size or just the permitted region within the allocation.
		}
		if (outOfBound) {
			string_append(message, U"|  Claimed memory is outside of the pointer's permitted memory region!\n");
		} else {
			string_append(message, U"|  Claimed memory is safely within the permitted memory region.\n");
		}
		string_append(message, U"|    Permitted region: ", (uintptr_t)regionStart, U" to ", (uintptr_t)regionEnd, U" of ", (intptr_t)(regionEnd - regionStart), U" bytes\n");
		string_append(message, U"|    Requested region: ", (uintptr_t)pointer, U" to ", ((uintptr_t)pointer) + claimedSize, U" of ", claimedSize, U" bytes\n");
		string_append(message, U"|    Element size: ", elementSize, U" bytes\n");
		string_append(message, U"\\_______________________________________________________________________\n\n");
		throwError(message);
	}

	void dsr::impl_assertInsideSafePointer(const char* method, const char* name, const uint8_t* pointer, const uint8_t* regionStart, const uint8_t* regionEnd, const AllocationHeader *header, uint64_t allocationIdentity, intptr_t claimedSize, intptr_t elementSize) {
		if (regionStart == nullptr) {
			throwPointerError(U"SafePointer identity exception! Tried to use a null pointer.", method, name, pointer, regionStart, regionEnd, header, allocationIdentity, claimedSize, elementSize, 0, 0);
			return;
		}
		// If the pointer has an allocation header, check that the identity matches the one stored in the pointer.
		uint64_t headerIdentity = 0;
		uint64_t headerHash = 0;
		if (header != nullptr) {
			try {
				// Both allocation identity and thread hash may match by mistake, but in most of the cases this will give more information about why it happened.
				headerIdentity = header->allocationIdentity;
				headerHash = header->threadHash;
			} catch(...) {
				headerIdentity = 0;
				headerHash = 0;
				throwPointerError(U"SafePointer exception! Tried to access memory not available to the application.", method, name, pointer, regionStart, regionEnd, header, allocationIdentity, claimedSize, elementSize, headerIdentity, headerHash);
				return;
			}
			if (headerIdentity != allocationIdentity) {
				throwPointerError(U"SafePointer identity exception!", method, name, pointer, regionStart, regionEnd, header, allocationIdentity, claimedSize, elementSize, headerIdentity, headerHash);
				return;
			} else if (headerHash != ANY_THREAD_HASH && headerHash != currentThreadHash) {
				throwPointerError(U"SafePointer thread hash exception!", method, name, pointer, regionStart, regionEnd, header, allocationIdentity, claimedSize, elementSize, headerIdentity, headerHash);
				return;
			}
		}
		if (isOutOfBound(pointer, claimedSize, regionStart, regionEnd)) {
			throwPointerError(U"SafePointer out of bound exception!", method, name, pointer, regionStart, regionEnd, header, allocationIdentity, claimedSize, elementSize, headerIdentity, headerHash);
			return;
		}
	}
#endif
