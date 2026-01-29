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
#include "../settings.h"

#ifdef SAFE_POINTER_CHECKS
	#include <thread>
	#include <mutex>
#endif

using namespace dsr;

#ifdef SAFE_POINTER_CHECKS
	// Thread hash of memory without any specific owner.
	static uint64_t ANY_THREAD_HASH = 0xF986BA1496E872A5;

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

	void AllocationHeader::reuse(bool threadLocal, const char *name) {
		this->threadHash = threadLocal ? currentThreadHash : ANY_THREAD_HASH;
		this->allocationIdentity = createIdentity();
		this->name = name;
	}
#else
	AllocationHeader::AllocationHeader()
	: totalSize(0) {}

	// TODO: Avoid passing the debug name in release mode by placing these functions in memory.h.
	//       Create separate methods for getting the thread hash and the next allocation nonce.
	AllocationHeader::AllocationHeader(uintptr_t totalSize, bool threadLocal, const char *name)
	: totalSize(totalSize) {}

	void AllocationHeader::reuse(bool threadLocal, const char *name) {}
#endif

#ifdef SAFE_POINTER_CHECKS
	void dsr::impl_assertNonNegativeSize(intptr_t size) {
		if (size < 0) {
			throwError(U"Negative size of SafePointer!\n");
		}
	}

	static bool isOutOfBound(const uint8_t* claimedStart, const uint8_t* claimedEnd, const uint8_t* permittedStart, const uint8_t* permittedEnd) {
		return claimedStart < permittedStart || claimedEnd > permittedEnd;
	}

	static void throwPointerError(const ReadableString &title, const char* methodName, const char* pointerName, const FixedAscii<256> &allocationName, const uint8_t* claimedStart, const uint8_t* claimedEnd, intptr_t elementSize, const uint8_t* permittedStart, const uint8_t* permittedEnd, const AllocationHeader *pointerHeader, uint64_t allocationIdentity, uint64_t headerIdentity, uint64_t headerHash) {
		bool outOfBound = isOutOfBound(claimedStart, claimedEnd, permittedStart, permittedEnd);
		String *target = &(string_getPrintBuffer());
		string_clear(*target);
		string_append(*target, title, U"\n");
		string_append(*target, U" _______________________________________________________________________\n");
		string_append(*target, U"/\n");
		#ifdef BAN_IMPLICIT_ASCII_CONVERSION
			string_append(*target, U"|  SafePointer operation: ");
			impl_toStreamIndented_ascii(*target, methodName, U"");
			string_append(*target, U"\n");
			string_append(*target, U"|  Pointer name: ");
			impl_toStreamIndented_ascii(*target, pointerName, U"");
			string_append(*target, U"\n");
		#else
			string_append(*target, U"|  SafePointer operation: ", methodName, U"\n");
			string_append(*target, U"|  Pointer name: ", pointerName, U"\n");
		#endif
		#ifdef EXTRA_SAFE_POINTER_CHECKS
			if (pointerHeader != nullptr) {
				string_append(*target, U"|  Allocation name    : ", allocationName.getPointer(), U"\n");
				string_append(*target, U"|  Thread hash:\n");
				if (headerHash == ANY_THREAD_HASH) {
					string_append(*target, U"|    Shared with all threads\n");
				} else {
					string_append(*target, U"|    Owner thread     : ", headerHash, U"\n");
					string_append(*target, U"|    Calling thread   : ", currentThreadHash, U"\n");
				}
				string_append(*target, U"|  Identity:\n");
				string_append(*target, U"|    Found            : ", headerIdentity, U"\n");
				string_append(*target, U"|    Expected         : ", allocationIdentity, U"\n");
				// TODO: Check if the requested data is outside of the memory allocation's used size or just the permitted region within the allocation.
				// TODO: Iterate over allocations using until the same header address as in the pointer is found:
				heap_forAllHeapAllocations([target, pointerHeader](AllocationHeader * header, void * allocation) {
					// We found the allocation in the heap, so we know that it is an active heap allocation.
					if (pointerHeader == header) {
						// The allocation size is the space that can be expanded into without having to reallocate.
						string_append(*target, U"|    Allocation size  : ", heap_getAllocationSize(allocation), U" bytes\n");
						// The used size is what the application asked for from the allocator.
						//   The permissed region often include the whole used size and some padding for aligned memory reads.
						string_append(*target, U"|    Used size        : ", heap_getUsedSize(allocation), U" bytes\n");
					}
				});
			}
		#endif
		if (outOfBound) {
			string_append(*target, U"|  Claimed memory is outside of the pointer's permitted memory region!\n");
		} else {
			string_append(*target, U"|  Claimed memory is safely within the permitted memory region.\n");
		}
		string_append(*target, U"|    Permitted region : ", (uintptr_t)permittedStart, U" to ", (uintptr_t)permittedEnd, U" of ", (intptr_t)(permittedEnd - permittedStart), U" bytes\n");
		string_append(*target, U"|    Requested region : ", (uintptr_t)claimedStart, U" to ", (uintptr_t)claimedEnd, U" of ", (uintptr_t)(claimedEnd - claimedStart), U" bytes\n");
		string_append(*target, U"|    Element size     : ", elementSize, U" bytes\n");
		string_append(*target, U"\\_______________________________________________________________________\n\n");
		string_sendMessage(*target, MessageType::Error);
	}

	static thread_local bool inside = false;
	void dsr::impl_assertInsideSafePointer(const char* methodName, const char* pointerName, const uint8_t* claimedStart, const uint8_t* claimedEnd, intptr_t elementSize, const uint8_t* permittedStart, const uint8_t* permittedEnd, const AllocationHeader *header, uint64_t allocationIdentity) {
		// Abort to avoid infinite recursion from printing text if we are already inside of another check.
		if (inside) return;
		inside = true;
		if (permittedStart == nullptr) {
			throwPointerError(U"SafePointer identity exception! Tried to use a null pointer.", methodName, pointerName, "(null)", claimedStart, claimedEnd, elementSize, permittedStart, permittedEnd, header, allocationIdentity, 0, 0);
			return;
		}
		// If the pointer has an allocation header, check that the identity matches the one stored in the pointer.
		uint64_t headerIdentity = 0;
		uint64_t headerHash = 0;
		FixedAscii<256> allocationName("(null)");
		#ifdef EXTRA_SAFE_POINTER_CHECKS
			if (header != nullptr) {
				#ifndef DSR_HARD_EXIT_ON_ERROR
				// This only works if the application has registered a signal handler throwing an error on SIGSEGV, like in the regression tests.
				try {
				#endif
					// Both allocation identity and thread hash may match by mistake, but in most of the cases this will give more information about why it happened.
					headerIdentity = header->allocationIdentity;
					headerHash = header->threadHash;
					if (header->name != nullptr) {
						// Clone into fixed size memory when we do not know if the memory is corrupted.
						allocationName = FixedAscii<256>(header->name);
					}
				#ifndef DSR_HARD_EXIT_ON_ERROR
				} catch(...) {
					headerIdentity = 0;
					headerHash = 0;
					throwPointerError(U"SafePointer exception! Tried to access memory not available to the application.", methodName, pointerName, "(invalid)", claimedStart, claimedEnd, elementSize, permittedStart, permittedEnd, header, allocationIdentity, headerIdentity, headerHash);
					return;
				}
				#endif
				if (headerIdentity != allocationIdentity) {
					throwPointerError(U"SafePointer identity exception!", methodName, pointerName, allocationName, claimedStart, claimedEnd, elementSize, permittedStart, permittedEnd, header, allocationIdentity, headerIdentity, headerHash);
					return;
				} else if (headerHash != ANY_THREAD_HASH && headerHash != currentThreadHash) {
					throwPointerError(U"SafePointer thread hash exception!", methodName, pointerName, allocationName, claimedStart, claimedEnd, elementSize, permittedStart, permittedEnd, header, allocationIdentity, headerIdentity, headerHash);
					return;
				}
			}
		#endif
		if (isOutOfBound(claimedStart, claimedEnd, permittedStart, permittedEnd)) {
			throwPointerError(U"SafePointer out of bound exception!", methodName, pointerName, allocationName, claimedStart, claimedEnd, elementSize, permittedStart, permittedEnd, header, allocationIdentity, headerIdentity, headerHash);
			return;
		}
		inside = false;
	}
#endif
