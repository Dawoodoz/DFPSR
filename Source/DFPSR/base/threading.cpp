﻿// zlib open source license
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

#include "threading.h"
#include "virtualStack.h"
#include "../implementation/math/scalar.h"

// Get settings from here.
#include "../settings.h"

#ifndef DISABLE_MULTI_THREADING
	// Requires -pthread for linking
	#include <thread>
	#include <mutex>
	#include <future>
#endif

namespace dsr {

#ifndef DISABLE_MULTI_THREADING
	static std::mutex getTaskLock;
#endif

int getThreadCount() {
	#ifndef DISABLE_MULTI_THREADING
		return (int)std::thread::hardware_concurrency();
	#else
		return 1;
	#endif
}

void threadedWorkByIndex(std::function<void(void *context, int jobIndex)> job, void *context, int jobCount, int maxThreadCount) {
	#ifdef DISABLE_MULTI_THREADING
		// Reference implementation
		for (int i = 0; i < jobCount; i++) {
			job(context, i);
		}
	#else
		if (jobCount <= 0) {
			return;
		} else if (jobCount == 1) {
			job(context, 0);
		} else {
			if (maxThreadCount <= 0) {
				// No limit.
				maxThreadCount = jobCount;
			}
			// When having more than one thread, one should be reserved for fast responses.
			//   Otherwise one thread will keep the others waiting while struggling to manage interrupts with expensive context switches.
			int availableThreads = max(getThreadCount() - 1, 1);
			int workerCount = min(availableThreads, maxThreadCount, jobCount); // All used threads
			int helperCount = workerCount - 1; // Excluding the main thread
			// Multi-threaded work loop
			if (workerCount == 1) {
				// Run on the main thread if there is only one.
				for (int i = 0; i < jobCount; i++) {
					job(context, i);
				}
			} else {
				// A shared counter protected by getTaskLock.
				int nextJobIndex = 0;
				DestructibleVirtualStackAllocation<std::function<void()>> workers(workerCount);
				DestructibleVirtualStackAllocation<std::future<void>> helpers(helperCount);
				for (int w = 0; w < workerCount; w++) {
					workers[w] = [&nextJobIndex, context, job, jobCount]() {
						while (true) {
							getTaskLock.lock();
							int taskIndex = nextJobIndex;
							nextJobIndex++;
							getTaskLock.unlock();
							if (taskIndex < jobCount) {
								job(context, taskIndex);
							} else {
								break;
							}
						}
					};
				}
				// Start working in the helper threads
				for (int h = 0; h < helperCount; h++) {
					helpers[h] = std::async(std::launch::async, workers[h]);
				}
				// Perform the same work on the main thread
				workers[workerCount - 1]();
				// Wait for all helpers to complete their work once all tasks have been handed out
				for (int h = 0; h < helperCount; h++) {
					if (helpers[h].valid()) {
						helpers[h].wait();
					}
				}
			}
		}
	#endif
}

void threadedWorkFromArray(std::function<void()>* jobs, int jobCount, int maxThreadCount) {
	#ifdef DISABLE_MULTI_THREADING
		// Reference implementation
		for (int i = 0; i < jobCount; i++) {
			jobs[i]();
		}
	#else
		if (jobCount <= 0) {
			return;
		} else if (jobCount == 1) {
			jobs[0]();
		} else {
			if (maxThreadCount <= 0) {
				// No limit.
				maxThreadCount = jobCount;
			}
			// When having more than one thread, one should be reserved for fast responses.
			//   Otherwise one thread will keep the others waiting while struggling to manage interrupts with expensive context switches.
			int availableThreads = max(getThreadCount() - 1, 1);
			int workerCount = min(availableThreads, maxThreadCount, jobCount); // All used threads
			int helperCount = workerCount - 1; // Excluding the main thread
			// Multi-threaded work loop
			if (workerCount == 1) {
				// Run on the main thread if there is only one.
				for (int i = 0; i < jobCount; i++) {
					jobs[i]();
				}
			} else {
				// A shared counter protected by getTaskLock.
				int nextJobIndex = 0;
				DestructibleVirtualStackAllocation<std::function<void()>> workers(workerCount);
				DestructibleVirtualStackAllocation<std::future<void>> helpers(helperCount);
				for (int w = 0; w < workerCount; w++) {
					workers[w] = [&nextJobIndex, jobs, jobCount]() {
						while (true) {
							getTaskLock.lock();
							int taskIndex = nextJobIndex;
							nextJobIndex++;
							getTaskLock.unlock();
							if (taskIndex < jobCount) {
								jobs[taskIndex]();
							} else {
								break;
							}
						}
					};
				}
				// Start working in the helper threads
				for (int h = 0; h < helperCount; h++) {
					helpers[h] = std::async(std::launch::async, workers[h]);
				}
				// Perform the same work on the main thread
				workers[workerCount - 1]();
				// Wait for all helpers to complete their work once all tasks have been handed out
				for (int h = 0; h < helperCount; h++) {
					if (helpers[h].valid()) {
						helpers[h].wait();
					}
				}
			}
		}
	#endif
}

void threadedWorkFromArray(SafePointer<std::function<void()>> jobs, int jobCount, int maxThreadCount) {
	threadedWorkFromArray(jobs.getUnsafe(), jobCount, maxThreadCount);
}

void threadedWorkFromList(List<std::function<void()>> jobs, int maxThreadCount) {
	threadedWorkFromArray(&jobs[0], jobs.length(), maxThreadCount);
	jobs.clear();
}

void threadedSplit(int startIndex, int stopIndex, std::function<void(int startIndex, int stopIndex)> task, int minimumJobSize, int jobsPerThread) {
	#ifndef DISABLE_MULTI_THREADING
		int totalCount = stopIndex - startIndex;
		int maxJobs = totalCount / minimumJobSize;
		int jobCount = getThreadCount() * jobsPerThread;
		if (jobCount > maxJobs) { jobCount = maxJobs; }
		if (jobCount < 1) { jobCount = 1; }
	#else
		int jobCount = 1;
	#endif
	if (jobCount == 1) {
		// Too little work for multi-threading
		task(startIndex, stopIndex);
	} else {
		// Use multiple threads
		DestructibleVirtualStackAllocation<std::function<void()>> jobs(jobCount);
		int givenRow = startIndex;
		for (int s = 0; s < jobCount; s++) {
			int remainingJobs = jobCount - s;
			int remainingRows = stopIndex - givenRow;
			int y1 = givenRow; // Inclusive
			int taskSize = remainingRows / remainingJobs;
			givenRow = givenRow + taskSize;
			int y2 = givenRow; // Exclusive
			jobs[s] = [task, y1, y2]() {
				task(y1, y2);
			};
		}
		threadedWorkFromArray(jobs, jobCount);
	}
}

void threadedSplit_disabled(int startIndex, int stopIndex, std::function<void(int startIndex, int stopIndex)> task) {
	task(startIndex, stopIndex);
}

void threadedSplit(const IRect& bound, std::function<void(const IRect& bound)> task, int minimumRowsPerJob, int jobsPerThread) {
	#ifndef DISABLE_MULTI_THREADING
		int maxJobs = bound.height() / minimumRowsPerJob;
		int jobCount = getThreadCount() * jobsPerThread;
		if (jobCount > maxJobs) { jobCount = maxJobs; }
		if (jobCount < 1) { jobCount = 1; }
	#else
		int jobCount = 1;
	#endif
	if (jobCount == 1) {
		// Too little work for multi-threading
		task(bound);
	} else {
		// Use multiple threads
		DestructibleVirtualStackAllocation<std::function<void()>> jobs(jobCount);
		int givenRow = bound.top();
		for (int s = 0; s < jobCount; s++) {
			int remainingJobs = jobCount - s;
			int remainingRows = bound.bottom() - givenRow;
			int y1 = givenRow;
			int taskSize = remainingRows / remainingJobs;
			givenRow = givenRow + taskSize;
			IRect subBound = IRect(bound.left(), y1, bound.width(), taskSize);
			jobs[s] = [task, subBound]() {
				task(subBound);
			};
		}
		threadedWorkFromArray(jobs, jobCount);
	}
}

void threadedSplit_disabled(const IRect& bound, std::function<void(const IRect& bound)> task) {
	task(bound);
}

}

