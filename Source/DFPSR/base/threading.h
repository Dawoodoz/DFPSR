// zlib open source license
//
// Copyright (c) 2017 to 2019 David Forsgren Piuva
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
#include <functional>

namespace dsr {

// Executes every function in the array of jobs from jobs[0] to jobs[jobCount - 1].
void threadedWorkFromArray(std::function<void()>* jobs, int jobCount);

// Executes every function in the list of jobs.
//   Also clears the list when done.
void threadedWorkFromList(List<std::function<void()>> jobs);

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
void threadedSplit(int startIndex, int stopIndex, std::function<void(int startIndex, int stopIndex)> task, int minimumJobSize = 128, int jobsPerThread = 2);
// Use as a place-holder if you want to disable multi-threading but easily turn it on and off for comparing performance
void threadedSplit_disabled(int startIndex, int stopIndex, std::function<void(int startIndex, int stopIndex)> task);
// A more convenient version for images looping over a rectangular bound of pixels.
//   The same left and right sides are given to each sub-bound to make memory alignment easy.
//   The top and bottoms are subdivided so that memory access is simple for cache prediction.
void threadedSplit(const IRect& bound, std::function<void(const IRect& bound)> task, int minimumRowsPerJob = 128, int jobsPerThread = 2);
// Use as a place-holder if you want to disable multi-threading but easily turn it on and off for comparing performance
void threadedSplit_disabled(const IRect& bound, std::function<void(const IRect& bound)> task);

}

#endif

