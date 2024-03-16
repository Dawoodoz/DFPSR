
// This allocator does not have any header and can be disabled by not linking it into the application
// or by defining DISABLE_ALLOCATOR (usually with a -DDISABLE_ALLOCATOR compiler flag).
#ifndef DISABLE_ALLOCATOR

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>

static std::mutex allocationLock;

// Allocation head stored in the beginning of each allocation
// The caller's content begins alignedHeadSize bytes after the head
struct AllocationHead {
	AllocationHead* nextUnused = nullptr;
	uintptr_t contentSize = 0;
};
// A fixed byte offset to make sure that structures are still aligned in memory
//   Increase if values larger than this are stored in data structures using new and delete operations
static const uintptr_t alignedHeadSize = 16;
static_assert(sizeof(AllocationHead) <= alignedHeadSize, "Increase alignedHeadSize to the next power of two.\n");

static AllocationHead* createAllocation(size_t contentSize) {
	//printf("createAllocation head(%li) + %li bytes\n", alignedHeadSize, contentSize);
	// calloc is faster than malloc for small allocations by not having to fetch old data to the cache
	AllocationHead* allocation = (AllocationHead*)calloc(alignedHeadSize + contentSize, 1);
	allocation->nextUnused = nullptr;
	allocation->contentSize = contentSize;
	return allocation;
}

static void* getContent(AllocationHead* head) {
	return (void*)(((uintptr_t)head) + alignedHeadSize);
}

static AllocationHead* getHead(void* content) {
	return (AllocationHead*)(((uintptr_t)content) - alignedHeadSize);
}

// Garbage pile
struct GarbagePile {
	AllocationHead* pileHead; // Linked list using nextUnused in AllocationHead
	const size_t fixedBufferSize;
	~GarbagePile() {
		AllocationHead* current = this->pileHead;
		while (current != nullptr) {
			// Free and go to the next allocation
			AllocationHead* next = current->nextUnused;
			free(current);
			current = next;
		}
	}
	AllocationHead* getAllocation() {
		//printf("getAllocation head(%li) + %li bytes\n", alignedHeadSize, this->fixedBufferSize);
		if (pileHead != nullptr) {
			// Pop the first unused allocation
			AllocationHead* result = this->pileHead;
			this->pileHead = this->pileHead->nextUnused;
			result->nextUnused = nullptr;
			result->contentSize = this->fixedBufferSize;
			return result;
		} else {
			// Create a new allocation
			return createAllocation(this->fixedBufferSize);
		}
	}
	void recycleAllocation(AllocationHead* unused) {
		// Clear old data to make debugging easier
		memset(getContent(unused), 0, this->fixedBufferSize);
		// Push new allocation to the pile
		unused->nextUnused = this->pileHead;
		this->pileHead = unused;
	}
};

static GarbagePile garbagePiles[8] = {
  {nullptr, 16},
  {nullptr, 32},
  {nullptr, 64},
  {nullptr, 128},
  {nullptr, 256},
  {nullptr, 512},
  {nullptr, 1024},
  {nullptr, 2048}
};

static int getBufferIndex(size_t contentSize) {
	if (contentSize <= 16) {
		return 0;
	} else if (contentSize <= 32) {
		return 1;
	} else if (contentSize <= 64) {
		return 2;
	} else if (contentSize <= 128) {
		return 3;
	} else if (contentSize <= 256) {
		return 4;
	} else if (contentSize <= 512) {
		return 5;
	} else if (contentSize <= 1024) {
		return 6;
	} else if (contentSize <= 2048) {
		return 7;
	} else {
		return -1;
	}
}

void* operator new(size_t contentSize) {
	allocationLock.lock();
		int bufferIndex = getBufferIndex(contentSize);
		AllocationHead* head;
		if (bufferIndex == -1) {
			head = createAllocation(contentSize);
			//printf("Allocated %li bytes without a size group\n", contentSize);
		} else {
			//printf("Requested at least %li bytes from size group %i\n", contentSize, bufferIndex);
			head = garbagePiles[bufferIndex].getAllocation();
		}
	allocationLock.unlock();
    return getContent(head);
}

void operator delete(void* content) {
	allocationLock.lock();
		AllocationHead* head = getHead(content);
		int bufferIndex = getBufferIndex(head->contentSize);
		if (bufferIndex == -1) {
			free(head);
			//printf("Freed memory of size %li without a size group\n", head->contentSize);
		} else {
			garbagePiles[bufferIndex].recycleAllocation(head);
			//printf("Freed memory of size %li from size group %i\n", head->contentSize, bufferIndex);
		}
    allocationLock.unlock();
}

#endif
