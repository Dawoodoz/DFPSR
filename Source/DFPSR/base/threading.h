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

#ifndef DFPSR_THREADING
#define DFPSR_THREADING

#include "../../DFPSR/collection/List.h"
#include "../../DFPSR/math/IRect.h"
#include "TemporaryCallback.h"
#include "StorableCallback.h"

namespace dsr {

// Get the number of threads available.
int32_t getThreadCount();

// Calls the same job function with indices 0 to jobIndex - 1.
//   This removes the need for capturing the same data over and over again when each task is identical with a different index.
//   By using TemporaryCallback that simply points to existing stack memory, it also avoids having to heap allocate any closures.
void threadedWorkByIndex(const TemporaryCallback<void(void *context, int32_t jobIndex)> &job, void *context, int32_t jobCount, int32_t maxThreadCount = 0);

// Executes every function in the array of jobs from jobs[0] to jobs[jobCount - 1].
//   The maxThreadCount argument is the maximum number of threads to use when enough threads are available.
//     Letting maxThreadCount be 0 removes the limit and uses as many threads as possible, limited only by getThreadCount() - 1 and jobCount.
//     Letting maxThreadCount be 1 forces single-threaded execution on the calling thread.
//   Useful when each job to execute is different and you want the convenience of StorableCallback.
void threadedWorkFromArray(SafePointer<StorableCallback<void()>> jobs, int32_t jobCount, int32_t maxThreadCount = 0);
void threadedWorkFromArray(StorableCallback<void()>* jobs, int32_t jobCount, int32_t maxThreadCount = 0);

// Executes every function in the list of jobs.
//   Also clears the list when done.
void threadedWorkFromList(List<StorableCallback<void()>> jobs, int32_t maxThreadCount = 0);

// Calling the given function with sub-sets of the interval using multiple threads in parallel.
//   Useful when you have lots of tiny jobs that can be grouped together into larger jobs.
//     Otherwise the time to start a thread may exceed the cost of the computation.
//   startIndex is inclusive but stopIndex is exclusive.
//     X is within the interval iff startIndex <= X < stopIndex.
//   Warning!
//     * Only write to non-overlapping memory regions.
//       This may require aligning the data or using padding depending on how cache works on the target platform.
//       The longer the distance is, the safer it is against race conditions causing weird results.
//       You may however read from write-protected shared input in any way you want.
//         Because data that doesn't change cannot have race conditions.
//     * Do not use for manipulation of pointers, stack memory from the calling thread or anything where corrupted output may lead to a crash.
//       Drawing pixel values is okay, because a race condition would only be some noisy pixels that can be spotted and fixed.
//       Race conditions cannot be tested nor proven away, so assume that they will happen and do your best to avoid them.
void threadedSplit(int32_t startIndex, int32_t stopIndex, const TemporaryCallback<void(int32_t startIndex, int32_t stopIndex)> &task, int32_t minimumJobSize = 128, int32_t jobsPerThread = 2, int32_t maxThreadCount = 0);
// Use as a place-holder if you want to disable multi-threading but easily turn it on and off for comparing performance
void threadedSplit_disabled(int32_t startIndex, int32_t stopIndex, const TemporaryCallback<void(int32_t startIndex, int32_t stopIndex)> &task);
// A more convenient version for images looping over a rectangular bound of pixels.
//   The same left and right sides are given to each sub-bound to make memory alignment easy.
//   The top and bottoms are subdivided so that memory access is simple for cache prediction.
void threadedSplit(const IRect& bound, const TemporaryCallback<void(const IRect& bound)> &task, int32_t minimumRowsPerJob = 128, int32_t jobsPerThread = 2, int32_t maxThreadCount = 0);
// Use as a place-holder if you want to disable multi-threading but easily turn it on and off for comparing performance
void threadedSplit_disabled(const IRect& bound, const TemporaryCallback<void(const IRect& bound)> &task);

}

#endif

