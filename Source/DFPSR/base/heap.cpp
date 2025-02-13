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

// TODO: Apply thread safety to more memory operations.
//       heap_getUsedSize and heap_setUsedSize are often used together, forming a transaction without any mutex.

#include "../settings.h"
#if defined(USE_MICROSOFT_WINDOWS)	
	#include <Windows.h>
#elif defined(USE_MACOS)
	#include <sys/sysctl.h>
#elif defined(USE_LINUX)
	#include <stdio.h>
	#include <stdint.h>
	#include <stdlib.h>
#endif

#ifndef DISABLE_MULTI_THREADING
	// Requires -pthread for linking
	#include <thread>
	#include <mutex>
#endif

#include "heap.h"
#include "../api/stringAPI.h"
#include "../api/timeAPI.h"
#include <stdio.h>
#include <new>
#include "simd.h"

#ifdef SAFE_POINTER_CHECKS
	#define DSR_PRINT_CACHE_LINE_SIZE
	#define DSR_PRINT_NO_MEMORY_LEAK
#endif

namespace dsr {
	// The default cache line size is used when not known from asking the system.
	static const uintptr_t defaultCacheLineSize = 128;
	// There is no point in using a heap alignment smaller than the allocation heads, so we align to at least 64 bytes.
	static const uintptr_t minimumHeapAlignment = 64;
	#if defined(USE_LINUX)
		static uintptr_t getCacheLineSizeFromIndices(uintptr_t cpuIndex, uintptr_t cacheLevel) {
			char path[256];
			snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%i/cache/index%i/coherency_line_size", (int)cpuIndex, (int)cacheLevel);
			FILE *file = fopen(path, "r");
			if (file == nullptr) {
				return 0;
			}
			int cacheLineSize;
			if (fscanf(file, "%i", &cacheLineSize) != 1) {
				cacheLineSize = 0;
			}
			fclose(file);
			return uintptr_t(cacheLineSize);
		}
		static uintptr_t getCacheLineSize() {
			uintptr_t result = 0;
			uintptr_t cpuIndex = 0;
			uintptr_t cacheLevel = 0;
			while (true) {
				uintptr_t newSize = getCacheLineSizeFromIndices(cpuIndex, cacheLevel);
				if (newSize == 0) {
					if (cacheLevel == 0) {
						// CPU does not exist, so we are done.
						break;
					} else {
						// Cache level does not exist. Go to the next CPU.
						cpuIndex++;
						cacheLevel = 0;
					}
				} else {
					// We have found the cache line size for cpuIndex and cacheLevel, so we include it in a maximum.
					//printf("CPU %i cache level %i is %i.\n", (int)cpuIndex, (int)cacheLevel, (int)newSize);
					result = max(result, newSize);
					cacheLevel++;
				}
			}
			if (result <= 0) {
				result = defaultCacheLineSize;
				printf("WARNING! Failed to read cache line size from Linux system folders. The application might not be thread-safe.\n");
			}
			#ifdef DSR_PRINT_CACHE_LINE_SIZE
				printf("Detected a cache line width of %i bytes from reading Linux system folders.\n", (int)result);
			#endif
			return result;
		}
	#elif defined(USE_MACOS)
		static uintptr_t getCacheLineSize() {
			uintptr_t result = 0;
			int cacheLine;
			size_t size = sizeof(cacheLine);
			int mib[2] = {CTL_HW, HW_CACHELINE};
			int error = sysctl(mib, 2, &cacheLine, &size, NULL, 0);
			if(error == 0) {
				result = cacheLine;
				#ifdef DSR_PRINT_CACHE_LINE_SIZE
					printf("Detected a cache line width of %i bytes on MacOS by asking for HW_CACHELINE with sysctl.\n", (int)result);
				#endif
			} else {
				result = defaultCacheLineSize;
				printf("WARNING! Failed to read HW_CACHELINE on MacOS. The application might not be thread-safe.\n");
			}
			return result;
		}
	#elif defined(USE_MICROSOFT_WINDOWS)
		static uintptr_t getCacheLineSize() {
			DWORD bufferSize = 0;
			GetLogicalProcessorInformation(nullptr, &bufferSize);
			SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(bufferSize);
			uintptr_t result = 0;
			if (buffer == nullptr) {
				result = defaultCacheLineSize;
				printf("WARNING! Failed to allocate buffer for the call to GetLogicalProcessorInformation on MS-Windows. The application might not be thread-safe.\n");
			} else if (GetLogicalProcessorInformation(buffer, &bufferSize)) {
				SYSTEM_LOGICAL_PROCESSOR_INFORMATION *entry = buffer;
				SYSTEM_LOGICAL_PROCESSOR_INFORMATION *lastEntry = buffer + (bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)) - 1;
				while (entry <= lastEntry) {
					if (entry->Relationship == RelationCache) {
						uintptr_t currentLineSize = entry->Cache.LineSize;
						if (currentLineSize > result) {
							result = currentLineSize;
						}
					}
					entry++;
				}
				#ifdef DSR_PRINT_CACHE_LINE_SIZE
					printf("Detected a cache line width of %i bytes on MS-Windows by checking each RelationCache with GetLogicalProcessorInformation.\n", (int)result);
				#endif
			} else {
				result = defaultCacheLineSize;
				printf("WARNING! The call to GetLogicalProcessorInformation failed to get the cache line size on MS-Windows. The application might not be thread-safe.\n");
			}
			free(buffer);
			return result;
		}
	#else
		static uintptr_t getCacheLineSize() {
			printf("WARNING! The target platform does not have a method for detecting cache line width in DFPSR/base/heap.cpp.\n");
			return defaultCacheLineSize;
		}
	#endif

	static uintptr_t heapAlignmentAndMask = 0u;
	uintptr_t heap_getHeapAlignment() {
		static uintptr_t heapAlignment = 0u;
		if (heapAlignment == 0) {
			heapAlignment = getCacheLineSize();
			if (heapAlignment < DSR_FLOAT_VECTOR_SIZE) { heapAlignment = DSR_FLOAT_VECTOR_SIZE; }
			if (heapAlignment < minimumHeapAlignment) { heapAlignment = minimumHeapAlignment; }
			heapAlignmentAndMask = memory_createAlignmentAndMask(heapAlignment);
		}
		return heapAlignment;
	}
	static uintptr_t heap_getHeapAlignmentAndMask() {
		heap_getHeapAlignment();
		return heapAlignmentAndMask;
	}

	// Keep track of the program's state.
	enum class ProgramState {
		Starting, // A single thread does global construction without using any mutex.
		Running, // Any number of threads allocate and free memory.
		Terminating // A single thread does global destruction without using any mutex.
	};
	ProgramState programState = ProgramState::Starting;

	// Count threads.
	#ifndef DISABLE_MULTI_THREADING
		static uint64_t threadCount = 0u;
		static std::mutex threadLock;
		struct ThreadCounter {
			ThreadCounter() {
				threadLock.lock();
					threadCount++;
					if (threadCount > 1 && programState != ProgramState::Running) {
						if (programState == ProgramState::Starting) {
							printf("Tried to create another thread before construction of global variables was complete!\n");
						} else if (programState == ProgramState::Terminating) {
							printf("Tried to create another thread after destruction of global variables had begun!\n");
						}
					}
				threadLock.unlock();
			};
			~ThreadCounter() {
				threadLock.lock();
					threadCount--;
				threadLock.unlock();
			};
		};
		thread_local ThreadCounter threadCounter;
		static uint64_t getThreadCount() {
			uint64_t result = 0;
			threadLock.lock();
				result = threadCount;
			threadLock.unlock();
			return result;
		}
	#endif

	// Because locking is recursive, it is safest to just have one global mutex for allocating, freeing and manipulating use counters.
	//   Otherwise each use counter would need to store the thread identity and recursive depth for each heap allocation.
	#ifndef DISABLE_MULTI_THREADING
		static thread_local intptr_t lockDepth = 0;
		static std::mutex memoryLock;
	#endif

	inline void lockMemory() {
		#ifndef DISABLE_MULTI_THREADING
			// Only call the mutex within main.
			if (programState == ProgramState::Running) {
				if (lockDepth == 0) {
					memoryLock.lock();
				}
				lockDepth++;
			}
		#endif
	}

	inline void unlockMemory() {
		#ifndef DISABLE_MULTI_THREADING
			// Only call the mutex within main.
			if (programState == ProgramState::Running) {
				lockDepth--;
				assert(lockDepth >= 0);
				if (lockDepth == 0) {
					memoryLock.unlock();
				}
			}
		#endif
	}

	// Called before main, after global initiation completes.
	void heap_startingApplication() {
		// Once global initiation has completed, mutexes can be used for multi-threading.
		programState = ProgramState::Running;
	}

	// Called after main, before global termination begins.
	void heap_terminatingApplication() {
		#ifndef DISABLE_MULTI_THREADING
			// Wait for all other threads to terminate before closing the program.
			while (getThreadCount() > 1) {
				// Sleep for 10 milliseconds before trying again.
				time_sleepSeconds(0.01);
			}
		#endif
		// Once global termination begins, we can no longer use the mutex.
		//   The memory system will still get calls to free resources, which must be handled with a single thread.
		programState = ProgramState::Terminating;
	}

	// The total number of used heap allocations, excluding recycled memory.
	// Only accessed when defaultHeap.poolLock is locked.
	static intptr_t allocationCount = 0;

	using HeapFlag = uint16_t;
	using BinIndex = uint16_t;

	// The free function is hidden, because all allocations are forced to use reference counting,
	//   so that a hard exit can disable recursive calls to heap_free by incrementing all reference counters.
	static void heap_free(void * const allocation);

	// The bin size equals two to the power of binIndex multiplied by minimumHeapAlignment.
	//   This allow knowing the number of bins in compile time by leaving a number of unused bins specified by MIN_BIN_COUNT.
	inline constexpr uintptr_t getBinSize(BinIndex binIndex) {
		// Index 0 starts at the minimum alignment to know the number of bins in compile time, so some bins will be unused when the alignment is bigger.
		return (uintptr_t(1) << uintptr_t(binIndex)) * minimumHeapAlignment;
	}

	// Calculates the largest power of two allocation size that does not overflow a pointer on the target platform.
	constexpr int calculateBinCount() {
		intptr_t p = 0;
		while (true) {
			uintptr_t result = getBinSize(p);
			// Once the value overflows in uintptr_t we have found the index that can not be used, which is also the bin count when including index zero.
			if (result == 0) {
				return p;
			}
			p++;
		}
	}

	// The index of the last used bin.
	static const int MAX_BIN_COUNT = calculateBinCount();

	static BinIndex getBinIndex(uintptr_t minimumSize, intptr_t minimumBin) {
		for (intptr_t p = minimumBin; p < MAX_BIN_COUNT; p++) {
			uintptr_t result = getBinSize(p);
			if (result >= minimumSize) {
				return p;
			}
		}
		// Failed to match any bin.
		return -1;
	}

	// The index of the first used bin, which is also the number of unused bins.
	static const int MIN_BIN_COUNT = getBinIndex(heap_getHeapAlignment(), 0);

	static const HeapFlag heapFlag_recycled = 1 << 0;
	struct HeapHeader : public AllocationHeader {
		// Because nextRecycled and usedSize have mutually exclusive lifetimes, they can share memory location.
		union {
			// When allocated
			uintptr_t usedSize; // The actual size requested.
			// When recycled
			HeapHeader *nextRecycled = nullptr;
		};
		HeapDestructor destructor;
		uintptr_t useCount = 0; // How many handles that point to the data.
		// TODO: Allow the caller to access custom bit flags in allocations.
		// uint32_t userFlags = 0;
		HeapFlag flags = 0; // Flags use the heapFlag_ prefix.
		BinIndex binIndex = 0; // Recycling bin index to use when freeing the allocation.
		HeapHeader(uintptr_t totalSize)
		: AllocationHeader(totalSize, false, "Nameless heap allocation") {}
		inline uintptr_t getAllocationSize() {
			return getBinSize(this->binIndex);
		}
		inline uintptr_t getUsedSize() {
			if (this->isRecycled()) {
				return 0;
			} else {
				return this->usedSize;
			}
		}
		inline uintptr_t setUsedSize(uintptr_t size) {
			if (!(this->isRecycled())) {
				uintptr_t allocationSize = getAllocationSize();
				if (size > allocationSize) {
					// Failed to assign the size, so it is important to check the result.
					size = allocationSize;
				}
				this->usedSize = size;
				return size;
			} else {
				return 0;
			}
		}
		inline bool isRecycled() const {
			return (this->flags & heapFlag_recycled) != 0;
		}
		inline void makeRecycled() {
			this->flags |= heapFlag_recycled;
		}
		inline void makeUsed() {
			this->flags &= ~heapFlag_recycled;
		}
	};

	// TODO: Allow using the header directly for manipulation in the API, now that the offset is not known in compile time.
	inline uintptr_t heap_getHeapHeaderPaddedSize() {
		static uintptr_t heapHeaderPaddedSize = 0;
		if (heapHeaderPaddedSize == 0) {
			heapHeaderPaddedSize = memory_getPaddedSize(sizeof(HeapHeader), heap_getHeapAlignment());
		}
		return heapHeaderPaddedSize;
	}

	AllocationHeader *heap_getHeader(void * const allocation) {
		return (AllocationHeader*)((uint8_t*)allocation - heap_getHeapHeaderPaddedSize());
	}

	inline HeapHeader *headerFromAllocation(void const * const allocation) {
		return (HeapHeader *)((uint8_t*)allocation - heap_getHeapHeaderPaddedSize());
	}

	inline void *allocationFromHeader(HeapHeader * const header) {
		return ((uint8_t*)header) + heap_getHeapHeaderPaddedSize();
	}
	inline void const *allocationFromHeader(HeapHeader const * const header) {
		return ((uint8_t const *)header) + heap_getHeapHeaderPaddedSize();
	}

	#ifdef SAFE_POINTER_CHECKS
		void heap_setAllocationName(void * const allocation, const char *name) {
			if (allocation != nullptr) {
				HeapHeader *header = headerFromAllocation(allocation);
				header->name = name;
			}
		}

		const char * heap_getAllocationName(void * const allocation) {
			if (allocation == nullptr) {
				return "none";
			} else {
				HeapHeader *header = headerFromAllocation(allocation);
				return header->name;
			}
		}

		uintptr_t heap_getPaddedSize(void const * const allocation) {
			if (allocation == nullptr) {
				return 0;
			} else {
				HeapHeader *header = headerFromAllocation(allocation);
				return memory_getPaddedSize_usingAndMask(header->getUsedSize(), heap_getHeapAlignmentAndMask());
			}
		}
	#endif

	uintptr_t heap_getAllocationSize(void const * const allocation) {
		HeapHeader *header = headerFromAllocation(allocation);
		return getBinSize(header->binIndex);
	}

	uintptr_t heap_getUsedSize(void const * const allocation) {
		uintptr_t result = 0;
		if (allocation != nullptr) {
			HeapHeader *header = headerFromAllocation(allocation);
			result = header->getUsedSize();
		}
		return result;
	}

	uintptr_t heap_setUsedSize(void * const allocation, uintptr_t size) {
		uintptr_t result = 0;
		if (allocation != nullptr) {
			HeapHeader *header = headerFromAllocation(allocation);
			result = header->setUsedSize(size);
		}
		return result;
	}

	void heap_increaseUseCount(void const * const allocation) {
		if (allocation != nullptr) {
			HeapHeader *header = headerFromAllocation(allocation);
			lockMemory();
			header->useCount++;
			unlockMemory();
		}
	}

	void heap_decreaseUseCount(void const * const allocation) {
		if (allocation != nullptr) {
			HeapHeader *header = headerFromAllocation(allocation);
			lockMemory();
			if (header->useCount == 0) {
				printf("Heap error: Decreasing a count that is already zero!\n");
			} else {
				header->useCount--;
				if (header->useCount == 0) {
					heap_free((void*)allocation);
				}
			}
			unlockMemory();
		}
	}

	uintptr_t heap_getUseCount(void const * const allocation) {
		return headerFromAllocation(allocation)->useCount;
	}

	// A block of memory where heap data can be allocated.
	struct HeapMemory {
		HeapMemory *prevHeap = nullptr;
		uint8_t *top = nullptr; // The start of the arena, where the allocation pointer is when full.
		uint8_t *allocationPointer = nullptr; // The allocation pointer that moves from bottom to top when filling the arena.
		uint8_t *bottom = nullptr; // The end of the arena, where the allocation pointer is when empty.
		HeapMemory(uintptr_t size) {
			this->top = (uint8_t*)(operator new (size));
			this->bottom = this->top + size;
			this->allocationPointer = this->bottom;
		}
		~HeapMemory() {
			if (this->top != nullptr) {
				delete this->top;
				this->top = nullptr;
			}
			this->allocationPointer = nullptr;
			this->bottom = nullptr;
		}
	};

	// The heap can have memory freed after its own destruction by telling the remaining allocations to clean up after themselves.
	struct HeapPool {
		HeapMemory *lastHeap = nullptr;
		HeapHeader *recyclingBin[MAX_BIN_COUNT] = {};
		void cleanUp() {
			// If memory safety checks are enabled, then we should indicate that everything is fine with the memory once cleaning up.
			//   There is however no way to distinguish between leaking memory and not yet having terminated everything, so there is no leak warning to print.
			#ifdef DSR_PRINT_NO_MEMORY_LEAK
				// Can't allocate more memory after freeing all memory.
				printf("All heap memory was freed without leaks.\n");
			#endif
			HeapMemory *nextHeap = this->lastHeap;
			while (nextHeap != nullptr) {
				HeapMemory *currentHeap = nextHeap;
				nextHeap = currentHeap->prevHeap;
				operator delete(currentHeap);
			}
			this->lastHeap = nullptr;
		}
		HeapPool() {}
		~HeapPool() {
			// The destruction should be ignored to manually terminate once all memory allocations have been freed.
			if (programState != ProgramState::Terminating) {
				printf("Heap error: Terminated the application without first calling heap_terminatingApplication or heap_hardExitCleaning!\n");
			}
		}
	};

	static UnsafeAllocation tryToAllocate(HeapMemory &heap, uintptr_t paddedSize, uintptr_t alignmentAndMask) {
		UnsafeAllocation result(nullptr, nullptr);
			uint8_t *dataPointer = (uint8_t*)(((uintptr_t)(heap.allocationPointer) - paddedSize) & alignmentAndMask);
			AllocationHeader *headerPointer = (AllocationHeader*)(dataPointer - heap_getHeapHeaderPaddedSize());
			if ((uint8_t*)headerPointer >= heap.top) {
				// There is enough space, so confirm the allocation.
				result = UnsafeAllocation(dataPointer, headerPointer);
				// Write data to the header.
				*headerPointer = HeapHeader((uintptr_t)heap.allocationPointer - (uintptr_t)headerPointer);
				// Reserve the data in the heap by moving the allocation pointer.
				heap.allocationPointer = (uint8_t*)headerPointer;
			}
		return result;
	}

	static UnsafeAllocation tryToAllocate(HeapPool &pool, uintptr_t paddedSize, uintptr_t alignmentAndMask) {
		// Start with the most recent heap, which is most likely to have enough space.
		HeapMemory *currentHeap = pool.lastHeap;
		while (currentHeap != nullptr) {
			UnsafeAllocation result = tryToAllocate(*currentHeap, paddedSize, alignmentAndMask);
			if (result.data != nullptr) {
				return result;
			}
			currentHeap = currentHeap->prevHeap;
		}
		// If no free space could be found, allocate a new memory block.
		uintptr_t allocationSize = 16777216;
		uintptr_t usefulSize = paddedSize << 4; // TODO: Handle integer overflow.
		if (usefulSize > allocationSize) allocationSize = usefulSize;
		// Append to the linked list of memory.
		HeapMemory *previousHeap = pool.lastHeap;
		pool.lastHeap = new HeapMemory(allocationSize);
		pool.lastHeap->prevHeap = previousHeap;
		// Make one last attempt at allocating the memory.
		return tryToAllocate(*(pool.lastHeap), paddedSize, alignmentAndMask);
	}

	static HeapPool defaultHeap;

	UnsafeAllocation heap_allocate(uintptr_t minimumSize, bool zeroed) {
		UnsafeAllocation result(nullptr, nullptr);
		int32_t binIndex = getBinIndex(minimumSize, MIN_BIN_COUNT);
		if (binIndex == -1) {
			// If the requested allocation is so big that there is no power of two that can contain it without overflowing the address space, then it can not be allocated.
			printf("Heap error: Exceeded the maximum size when trying to allocate!\n");
		} else {
			uintptr_t paddedSize = getBinSize(binIndex);
			lockMemory();
				allocationCount++;
				// Look for pre-existing allocations in the recycling bins.
				HeapHeader *binHeader = defaultHeap.recyclingBin[binIndex];
				if (binHeader != nullptr) {
					// Make the recycled allocation's tail into the new head.
					defaultHeap.recyclingBin[binIndex] = binHeader->nextRecycled;
					// Clear the pointer to make room for the allocation's size in the union.
					binHeader->nextRecycled = nullptr;
					// Mark the allocation as not recycled. (assume that it was recycled when found in the bin)
					binHeader->makeUsed();
					binHeader->reuse(false, "Nameless reused allocation");
					result = UnsafeAllocation((uint8_t*)allocationFromHeader(binHeader), binHeader);
				} else {
					// Look for a heap with enough space for a new allocation.
					result = tryToAllocate(defaultHeap, paddedSize, heap_getHeapAlignmentAndMask());
					if (result.data == nullptr) {
						printf("Heap error: Failed to allocate more memory!\n");
					}
				}
			unlockMemory();
			if (zeroed && result.data != nullptr) {
				memset(result.data, 0, paddedSize);
			}
		}
		if (result.data != nullptr) {
			// Get the header.
			HeapHeader *head = (HeapHeader*)(result.header);
			// Tell the allocation where it should be recycled when freed.
			head->binIndex = binIndex;
			// Tell the allocation how many of the bytes that are used.
			head->setUsedSize(minimumSize);
		}
		return result;
	}

	void heap_setAllocationDestructor(void * const allocation, const HeapDestructor &destructor) {
		HeapHeader *header = headerFromAllocation(allocation);		
		header->destructor = destructor;
	}

	static void heap_free(void * const allocation) {
		lockMemory();
			// Get the recycled allocation's header.
			HeapHeader *header = headerFromAllocation(allocation);
			if (header->isRecycled()) {
				printf("Heap error: A heap allocation was freed twice!\n");
			} else {
				// Call the destructor provided with any external resource that also needs to be freed.
				if (header->destructor.destructor) {
					header->destructor.destructor(allocation, header->destructor.externalResource);
				}
				// Remove the destructor so that it is not called again for the next allocation.
				header->destructor = HeapDestructor();
				int binIndex = header->binIndex;
				if (binIndex >= MAX_BIN_COUNT) {
					printf("Heap error: Out of bound recycling bin index in corrupted head of freed allocation!\n");
				} else {
					// Make any previous head from the bin into the new tail.
					HeapHeader *oldHeader = defaultHeap.recyclingBin[binIndex];
					header->nextRecycled = oldHeader;
					// Mark the allocation as recycled.
					header->makeRecycled();
					#ifdef SAFE_POINTER_CHECKS
						// Remove the allocation identity, so that use of freed memory can be detected in SafePointer and Handle.
						header->allocationIdentity = 0;
						header->threadHash = 0;
					#endif
					// Store the newly recycled allocation in the bin.
					defaultHeap.recyclingBin[binIndex] = header;
					header->nextRecycled = oldHeader;
				}
			}
			// By decreasing the count after recursive calls to destructors, we can make sure that the arena is freed last.
			// If a destructor allocates new memory, it will have to allocate a new arena and then clean it up again.
			allocationCount--;
			// If the heap has been told to terminate and we reached zero allocations, we can tell it to clean up.
			if (programState == ProgramState::Terminating && allocationCount == 0) {
				defaultHeap.cleanUp();
			}
		unlockMemory();
	}

	static void forAllHeapAllocations(HeapMemory &heap, std::function<void(AllocationHeader * header, void * allocation)> callback) {
		uint8_t * current = heap.allocationPointer;
		while (current < heap.bottom) {
			HeapHeader *header = (HeapHeader*)current;
			void *payload = allocationFromHeader(header);
			if (!(header->isRecycled())) {
				callback(header, payload);
			}
			current += header->totalSize;
		}
	}

	void heap_forAllHeapAllocations(std::function<void(AllocationHeader * header, void * allocation)> callback) {
		HeapMemory *currentHeap = defaultHeap.lastHeap;
		while (currentHeap != nullptr) {
			forAllHeapAllocations(*currentHeap, callback);
			currentHeap = currentHeap->prevHeap;
		}
	}

	void heap_hardExitCleaning() {
		// TODO:
		// * Increment the use count for each allocation, to prevent recursive freeing of resources.
		// * Call the destructor on each allocation without freeing any memory, while all memory is still available.
		// Then the arenas can safely be deallocated without looking at individual allocations again.
		allocationCount = 0;
		programState = ProgramState::Terminating;
		defaultHeap.cleanUp();
	}

	void impl_throwAllocationFailure() {
		string_sendMessage(U"Failed to allocate memory for a new object!\n", MessageType::Error);
	}

	void impl_throwNullException() {
		string_sendMessage(U"Null handle exception!\n", MessageType::Error);
	}

	void impl_throwIdentityMismatch(uint64_t allocationIdentity, uint64_t pointerIdentity) {
		throwError(U"Identity mismatch! The allocation pointed to had identity ", allocationIdentity, U" but ", pointerIdentity, U" was expected by the pointer from when it was allocated.\n");
	}
}
